#include "HomesteadSmokeTest.h"

#include "HomesteadController.h"
#include "HomesteadSave.h"
#include "HomesteadTestPaths.h"
#include "HomesteadWorld.h"
#include "Dom/JsonObject.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"
#include "ProceduralMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include <algorithm>
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

// Reuse the native resume fixture's local-file/reparse-point admission.
namespace HomesteadNativeMenuProof { bool OrdinaryLocalPath(FString Path, bool Directory); }

namespace
{
using namespace Homestead;

struct FWoodlandFixture
{
    Generation::WorldDescriptor World;
    Generation::GeneratedEntityKey SiteTree;
    int32 BuildX = 0, BuildY = 0, PlotX = 0, PlotY = 0;
    TArray<ResourceNode> Clear;
    Point WalkStart{-3750, 0};
    FVector WalkBefore = FVector::ZeroVector, WalkAfter = FVector::ZeroVector;
    FVector Previous = FVector::ZeroVector;
    bool CrossedOldBoundary = false, CrossedChunkSeam = false, ContinuousGround = true;
    size_t EditCount = 0;
    FString WorldId, SimulationText, Fingerprint;
    FString ActiveBatchSnapshot;
    int32 ActiveBatchComponents = 0, ActiveBatchInstances = 0, ActiveCollisionCapsules = 0;
    FString OuterBatchSnapshot;
    int32 OuterBatchComponents = 0, OuterBatchInstances = 0;
    uint32 ProducerProcess = 0;
};

double DistanceSquared(Point A, Point B)
{
    return FMath::Square(A.x - B.x) + FMath::Square(A.y - B.y);
}

bool Blocks(const ResourceNode& Node, int32 X, int32 Y)
{
    if (Node.cleared) return false;
    if (Node.kind == ResourceKind::Sapling)
        return FMath::FloorToInt(Node.position.x / CellSize) == X
            && FMath::FloorToInt(Node.position.y / CellSize) == Y;
    if (Node.kind != ResourceKind::ForestTree) return false;
    return DistanceSquared(Node.position, {
        FMath::Clamp(Node.position.x, X * CellSize, (X + 1) * CellSize),
        FMath::Clamp(Node.position.y, Y * CellSize, (Y + 1) * CellSize)}) <= 2500;
}

const Plot* FixturePlot(const State& State, const FWoodlandFixture& Fixture)
{
    for (const auto& Plot : State.plots)
        if (Plot.cellX == Fixture.PlotX && Plot.cellY == Fixture.PlotY) return &Plot;
    return nullptr;
}

bool ClearedKey(const Simulation& Sim, Generation::GeneratedEntityKey Key, bool Active)
{
    ResourceNode Node;
    if (!Sim.ResolveGeneratedResource(Key, Node) || Node.kind != ResourceKind::ForestTree
        || !Node.cleared || Node.readyAtHour != 0 || (Active ? Node.id <= 0 : Node.id != 0)) return false;
    for (const auto& Edit : Sim.GetState().resourceEdits)
        if (Edit.key == Key) return Edit.cleared && Edit.readyAtHour == 0;
    return false;
}

bool SitePersists(const Simulation& Sim, const FWoodlandFixture& Fixture)
{
    const auto& State = Sim.GetState();
    if (State.world.seed != Fixture.World.seed
        || State.world.generationVersion != Fixture.World.generationVersion
        || !ClearedKey(Sim, Fixture.SiteTree, true))
        return false;
    bool Foundation = false, Fire = false;
    for (const auto& Structure : State.structures)
        if (Structure.cellX == Fixture.BuildX && Structure.cellY == Fixture.BuildY)
        {
            Foundation |= Structure.kind == Piece::Foundation;
            Fire |= Structure.kind == Piece::Fire && Structure.fuelHours > 0;
        }
    const auto* Plot = FixturePlot(State, Fixture);
    return Foundation && Fire && Plot && Plot->planted && Plot->kind == CropKind::Roots
        && Plot->moisture > 0.8 && Sim.Count(Item::RoastedRoots) == 1;
}

bool ShadersReady()
{
#if WITH_EDITOR
    if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) return false;
#endif
    return true;
}

bool ChooseSite(const Simulation& Sim, FWoodlandFixture& Fixture)
{
    const auto& State = Sim.GetState();
    double Best = TNumericLimits<double>::Max();
    for (const auto& Tree : State.resources)
    {
        if (Tree.kind != ResourceKind::ForestTree || Tree.cleared
            || DistanceSquared(Tree.position, {-1000, 0}) > FMath::Square(3200.0)) continue;
        const int32 X = FMath::FloorToInt(Tree.position.x / CellSize);
        const int32 Y = FMath::FloorToInt(Tree.position.y / CellSize);
        auto CellRelief = [&State](int32 CellX, int32 CellY)
        {
            double Low = TNumericLimits<double>::Max(), High = -TNumericLimits<double>::Max();
            for (int32 DY : {0, 1})
                for (int32 DX : {0, 1})
                {
                    Generation::TerrainSample Sample;
                    if (Generation::SampleTerrain(State.world,
                        static_cast<int64>(CellX + DX) * static_cast<int64>(CellSize),
                        static_cast<int64>(CellY + DY) * static_cast<int64>(CellSize), Sample)
                        != Generation::Status::Ok) return TNumericLimits<double>::Max();
                    Low = FMath::Min(Low, Sample.heightCm);
                    High = FMath::Max(High, Sample.heightCm);
                }
            return High - Low;
        };
        if (CellRelief(X, Y) > 80) continue;
        const FIntPoint GardenOffsets[] = {{0,1},{0,-1},{1,0},{-1,0},{0,2},{0,-2},{2,0},{-2,0}};
        for (const FIntPoint GardenOffset : GardenOffsets)
        {
            const int32 GardenX = X + GardenOffset.X, GardenY = Y + GardenOffset.Y;
            if (CellRelief(GardenX, GardenY) > 80) continue;
            const Point Build = CellCenter(X, Y), Garden = CellCenter(GardenX, GardenY);
            const Point TillApproach{Garden.x - 190, Garden.y};
            TArray<ResourceNode> Clear;
            for (const auto& Node : State.resources)
                if (!Node.cleared && (Blocks(Node, X, Y) || Blocks(Node, GardenX, GardenY)
                    || DistanceSquared(Node.position, TillApproach) < FMath::Square(300.0)
                    || DistanceSquared(Node.position, {Build.x - 350, Build.y}) < FMath::Square(140.0)
                    || DistanceSquared(Node.position, {Build.x - 200, Build.y}) < FMath::Square(220.0)))
                    Clear.Add(Node);
            if (Clear.Num() > 2 || !Clear.ContainsByPredicate(
                [&Tree](const ResourceNode& Node) { return Node.key == Tree.key; })) continue;
            const double Score = Clear.Num() * 10000000.0 + DistanceSquared(Build, {-1000, 0});
            if (Score >= Best) continue;
            Best = Score;
            Fixture.World = State.world;
            Fixture.SiteTree = Tree.key;
            Fixture.BuildX = X; Fixture.BuildY = Y;
            Fixture.PlotX = GardenX; Fixture.PlotY = GardenY;
            Fixture.Clear = MoveTemp(Clear);
        }
    }
    return Best != TNumericLimits<double>::Max();
}

bool ChooseWalk(const Simulation& Sim, FWoodlandFixture& Fixture)
{
    // A bounded, read-only corridor survey, not repeated attempts at a failed walk.
    for (int32 Y = 0; Y >= -2000; Y -= 400)
    {
        bool Clear = true;
        for (int32 CY = -2; CY <= 0; ++CY)
            for (int32 CX = -3; CX <= -2; ++CX)
            {
                Generation::ChunkBaseline Chunk;
                if (Generation::GenerateChunk(Sim.GetState().world, {CX, CY}, Chunk)
                    != Generation::Status::Ok) return false;
                for (const auto& Entity : Chunk.entities)
                {
                    ResourceNode Node;
                    const auto Resolved = Sim.ResolveGeneratedResource(Entity.key, Node);
                    if (!Resolved)
                    {
                        if (Resolved.code == ResultCode::Unavailable) continue;
                        return false;
                    }
                    if (Node.cleared || (Node.kind != ResourceKind::ForestTree
                        && Node.kind != ResourceKind::Sapling)) continue;
                    if (DistanceSquared(Node.position, {
                        FMath::Clamp(Node.position.x, -5550.0, -3650.0), static_cast<double>(Y)})
                        < FMath::Square(115.0)) Clear = false;
                }
            }
        if (Clear) { Fixture.WalkStart = {-3750, static_cast<double>(Y)}; return true; }
    }
    return false;
}

TSharedRef<FJsonObject> ProofJson(const FWoodlandFixture& Fixture)
{
    auto Json = MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("route"), TEXT("HomesteadGeneratedWoodland"));
    Json->SetNumberField(TEXT("schema"), 1);
    Json->SetNumberField(TEXT("producerProcess"), Fixture.ProducerProcess);
    Json->SetStringField(TEXT("world"), Fixture.WorldId);
    Json->SetStringField(TEXT("seed"), UTF8_TO_TCHAR(std::to_string(Fixture.World.seed).c_str()));
    Json->SetNumberField(TEXT("generationVersion"), Fixture.World.generationVersion);
    Json->SetStringField(TEXT("simulation"), Fixture.SimulationText);
    Json->SetStringField(TEXT("saveMd5"), Fixture.Fingerprint);
    Json->SetNumberField(TEXT("buildX"), Fixture.BuildX); Json->SetNumberField(TEXT("buildY"), Fixture.BuildY);
    Json->SetNumberField(TEXT("plotX"), Fixture.PlotX); Json->SetNumberField(TEXT("plotY"), Fixture.PlotY);
    for (const auto& Entry : {TPair<FString, Generation::GeneratedEntityKey>(TEXT("siteTree"), Fixture.SiteTree)})
    {
        auto Key = MakeShared<FJsonObject>();
        Key->SetNumberField(TEXT("x"), Entry.Value.chunk.x);
        Key->SetNumberField(TEXT("y"), Entry.Value.chunk.y);
        Key->SetNumberField(TEXT("localId"), Entry.Value.localId);
        Json->SetObjectField(Entry.Key, Key);
    }
    Json->SetNumberField(TEXT("walkBeforeX"), Fixture.WalkBefore.X);
    Json->SetNumberField(TEXT("walkAfterX"), Fixture.WalkAfter.X);
    Json->SetNumberField(TEXT("walkY"), Fixture.WalkStart.y);
    Json->SetBoolField(TEXT("crossedOldBoundary"), Fixture.CrossedOldBoundary);
    Json->SetBoolField(TEXT("crossedChunkSeam"), Fixture.CrossedChunkSeam);
    Json->SetBoolField(TEXT("continuousGround"), Fixture.ContinuousGround);
    Json->SetStringField(TEXT("classification"), TEXT("CONTROLLED: supplied tools/materials/water; setup/return teleports; mapped-input walk and actions"));
    return Json;
}

bool ReadProof(const FString& Producer, FWoodlandFixture& Fixture, TArray<uint8>& Bytes)
{
    FString Directory = Producer, Consumer = HomesteadTestOutputDirectory();
    FPaths::NormalizeDirectoryName(Directory);
    if (!FPaths::CollapseRelativeDirectories(Directory)) return false;
    FPaths::NormalizeDirectoryName(Consumer);
    const FString Segment(TEXT("/Saved/Automation/"));
    const int32 RootEnd = Consumer.Find(Segment, ESearchCase::IgnoreCase);
    if (FPaths::IsRelative(Producer) || RootEnd < 0
        || !FPaths::IsUnderDirectory(Directory, Consumer.Left(RootEnd + Segment.Len()))
        || FPaths::IsSamePath(Directory, Consumer)
        || !HomesteadNativeMenuProof::OrdinaryLocalPath(Directory, true)) return false;
    const FString Proof = FPaths::Combine(Directory, TEXT("generated-woodland-fixture.json"));
    const FString Save = FPaths::Combine(Directory, TEXT("generated-woodland-fixture.sav"));
    if (!HomesteadNativeMenuProof::OrdinaryLocalPath(Proof, false)
        || !HomesteadNativeMenuProof::OrdinaryLocalPath(Save, false)
        || IFileManager::Get().FileSize(*Proof) > 8 * 1024 * 1024
        || IFileManager::Get().FileSize(*Save) > 20 * 1024 * 1024) return false;
    FString Text, Route, Seed;
    TSharedPtr<FJsonObject> Json;
    if (!FFileHelper::LoadFileToString(Text, *Proof)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Json) || !Json
        || !FFileHelper::LoadFileToArray(Bytes, *Save)) return false;
    auto Integer = [&Json](const TCHAR* Name, int64 Low, int64 High, int64& Out)
    {
        double Number = 0;
        if (!Json->TryGetNumberField(Name, Number) || !FMath::IsFinite(Number)
            || Number < Low || Number > High || Number != FMath::FloorToDouble(Number)) return false;
        Out = static_cast<int64>(Number); return true;
    };
    int64 Process = 0, Schema = 0, Version = 0;
    if (!Json->TryGetStringField(TEXT("route"), Route) || Route != TEXT("HomesteadGeneratedWoodland")
        || !Integer(TEXT("schema"), 1, 1, Schema)
        || !Integer(TEXT("producerProcess"), 1, MAX_uint32, Process)
        || Process == FPlatformProcess::GetCurrentProcessId()
        || !Integer(TEXT("generationVersion"), Generation::WorldGenerationVersion,
            Generation::WorldGenerationVersion, Version)
        || !Json->TryGetStringField(TEXT("seed"), Seed)
        || !Json->TryGetStringField(TEXT("world"), Fixture.WorldId)
        || !Json->TryGetStringField(TEXT("simulation"), Fixture.SimulationText)
        || !Json->TryGetStringField(TEXT("saveMd5"), Fixture.Fingerprint)
        || Fixture.Fingerprint != FMD5::HashBytes(Bytes.GetData(), Bytes.Num())) return false;
    Homestead::Simulation Parsed;
    if (!Parsed.Deserialize(TCHAR_TO_UTF8(*Fixture.SimulationText))
        || Seed != UTF8_TO_TCHAR(std::to_string(Parsed.GetState().world.seed).c_str())
        || Parsed.GetState().world.generationVersion != Version) return false;
    Fixture.World = Parsed.GetState().world;
    Fixture.ProducerProcess = static_cast<uint32>(Process);
    int32* Cells[] = {&Fixture.BuildX, &Fixture.BuildY, &Fixture.PlotX, &Fixture.PlotY};
    const TCHAR* Names[] = {TEXT("buildX"), TEXT("buildY"), TEXT("plotX"), TEXT("plotY")};
    for (int32 I = 0; I < 4; ++I)
    {
        int64 Value = 0;
        if (!Integer(Names[I], -3333, 3332, Value)) return false;
        *Cells[I] = static_cast<int32>(Value);
    }
    for (const auto& Entry : {TPair<FString, Generation::GeneratedEntityKey*>(TEXT("siteTree"), &Fixture.SiteTree)})
    {
        const TSharedPtr<FJsonObject>* Key = nullptr;
        if (!Json->TryGetObjectField(Entry.Key, Key) || !Key || !Key->IsValid()) return false;
        double X = 0, Y = 0, Id = 0;
        if (!(*Key)->TryGetNumberField(TEXT("x"), X) || !(*Key)->TryGetNumberField(TEXT("y"), Y)
            || !(*Key)->TryGetNumberField(TEXT("localId"), Id)
            || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Id)
            || X < -417 || X > 416 || Y < -417 || Y > 416 || Id < 1 || Id > MAX_uint32
            || X != FMath::FloorToDouble(X) || Y != FMath::FloorToDouble(Y) || Id != FMath::FloorToDouble(Id)) return false;
        *Entry.Value = {{static_cast<int32>(X), static_cast<int32>(Y)}, static_cast<uint32>(Id)};
    }
    ResourceNode SiteTree;
    return Parsed.ResolveGeneratedResource(Fixture.SiteTree, SiteTree)
        && FMath::FloorToInt(SiteTree.position.x / CellSize) == Fixture.BuildX
        && FMath::FloorToInt(SiteTree.position.y / CellSize) == Fixture.BuildY
        && SitePersists(Parsed, Fixture);
}
}

void AHomesteadSmokeTest::PrepareGeneratedWorldChecks()
{
    const auto Fixture = MakeShared<FWoodlandFixture>();
    auto Capture = [this](const FString& Name)
    {
        Add(TEXT("Settle integrated generated-world materials before ") + Name,
            []() {}, [this]() { return ShadersReady() && VerifyPresentationMaterials()
                && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
                && !Controller->IsBookOpen() && !Controller->IsPlanning(); }, 5.0f);
        Add(TEXT("Capture actual gameplay camera: ") + Name,
            [this, Name]() { Screenshot(Name); },
            [this]() { return ShadersReady() && VerifyPresentationMaterials()
                && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0; }, 0.8f);
    };
    auto OuterBatchSnapshot = [this](int32& ComponentCount, int32& InstanceCount)
    {
        ComponentCount = 0;
        InstanceCount = 0;
        if (!Controller->Landscape) return FString();
        TArray<FString> Rows;
        for (const auto& Entry : Controller->Landscape->OuterTreeInstances)
            Rows.Add(Entry.Key + TEXT("|") + Entry.Value.MeshPath + TEXT("|") + Entry.Value.Transform.ToString());
        Rows.Sort();
        for (const auto& Entry : Controller->Landscape->OuterTreeBatches)
        {
            if (!IsValid(Entry.Value.Get()) || !Entry.Value->IsRegistered()
                || !Entry.Value->GetStaticMesh() || Entry.Value->GetStaticMesh()->GetPathName() != Entry.Key
                || Entry.Value->IsQueryCollisionEnabled() || Entry.Value->GetGenerateOverlapEvents()
                || Entry.Value->CanEverAffectNavigation())
                return FString();
            ++ComponentCount;
            InstanceCount += Entry.Value->GetInstanceCount();
            for (int32 Index = 0; Index < Entry.Value->GetInstanceCount(); ++Index)
            {
                FTransform Transform;
                if (!Entry.Value->GetInstanceTransform(Index, Transform)) return FString();
                Rows.Add(TEXT("B|") + Entry.Key + TEXT("|") + Transform.ToString());
            }
        }
        TArray<UHierarchicalInstancedStaticMeshComponent*> Registered;
        Controller->Landscape->GetComponents(Registered);
        Registered.RemoveAll([](const UHierarchicalInstancedStaticMeshComponent* Batch)
        {
            return !Batch || !Batch->ComponentHasTag(TEXT("GeneratedOuterTreeBatch"));
        });
        if (Registered.Num() != ComponentCount
            || InstanceCount != Controller->Landscape->OuterTreeInstances.Num())
            return FString();
        return FString::Join(Rows, TEXT("\n"));
    };
    auto ActiveBatchSnapshot = [this](int32& ComponentCount, int32& InstanceCount, int32& CollisionCount)
    {
        ComponentCount = 0;
        InstanceCount = 0;
        CollisionCount = 0;
        if (!Controller->Landscape) return FString();
        TArray<FString> Rows;
        for (const auto& Entry : Controller->Landscape->ActiveTreeInstances)
            Rows.Add(Entry.Key + TEXT("|") + Entry.Value.Visual.MeshPath + TEXT("|")
                + Entry.Value.Visual.Transform.ToString() + TEXT("|")
                + Entry.Value.CollisionTransform.ToString());
        for (const auto& Entry : Controller->Landscape->ActiveTreeBatches)
        {
            if (!IsValid(Entry.Value.Get()) || !Entry.Value->IsRegistered()
                || !Entry.Value->GetStaticMesh() || Entry.Value->GetStaticMesh()->GetPathName() != Entry.Key
                || Entry.Value->IsQueryCollisionEnabled())
                return FString();
            ++ComponentCount;
            InstanceCount += Entry.Value->GetInstanceCount();
            for (int32 Index = 0; Index < Entry.Value->GetInstanceCount(); ++Index)
            {
                FTransform Transform;
                if (!Entry.Value->GetInstanceTransform(Index, Transform)) return FString();
                Rows.Add(TEXT("B|") + Entry.Key + TEXT("|") + Transform.ToString());
            }
        }
        for (const auto& Entry : Controller->Landscape->ActiveTreeCollisions)
        {
            if (!IsValid(Entry.Value.Get()) || !Entry.Value->IsRegistered()
                || !Entry.Value->ComponentHasTag(TEXT("GeneratedForestTreeCollision"))
                || !Entry.Value->ComponentHasTag(
                    FName(*(FString(TEXT("TreeKey_")) + Entry.Key)))
                || !Entry.Value->IsQueryCollisionEnabled()
                || Entry.Value->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block
                || Entry.Value->GetCollisionResponseToChannel(ECC_Camera) != ECR_Block)
                return FString();
            ++CollisionCount;
            Rows.Add(TEXT("C|") + Entry.Key + TEXT("|") + Entry.Value->GetRelativeTransform().ToString()
                + FString::Printf(TEXT("|%.6f|%.6f"), Entry.Value->GetUnscaledCapsuleRadius(),
                    Entry.Value->GetUnscaledCapsuleHalfHeight()));
        }
        TArray<UHierarchicalInstancedStaticMeshComponent*> RegisteredBatches;
        Controller->Landscape->GetComponents(RegisteredBatches);
        RegisteredBatches.RemoveAll([](const UHierarchicalInstancedStaticMeshComponent* Batch)
        {
            return !Batch || !Batch->ComponentHasTag(TEXT("GeneratedActiveTreeBatch"));
        });
        TArray<UCapsuleComponent*> RegisteredCollisions;
        Controller->Landscape->GetComponents(RegisteredCollisions);
        RegisteredCollisions.RemoveAll([](const UCapsuleComponent* Collision)
        {
            return !Collision || !Collision->ComponentHasTag(TEXT("GeneratedForestTreeCollision"));
        });
        if (RegisteredBatches.Num() != ComponentCount || RegisteredCollisions.Num() != CollisionCount
            || InstanceCount != Controller->Landscape->ActiveTreeInstances.Num()
            || CollisionCount != Controller->Landscape->ActiveTreeInstances.Num())
            return FString();
        Rows.Sort();
        return FString::Join(Rows, TEXT("\n"));
    };
    Add(TEXT("Generated woodland uses the isolated native test sandbox"),
        []() {},
        [this]()
        {
            return Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
                && Controller->SaveRoute.Mode == TEXT("test-sandbox")
                && FPaths::IsSamePath(Controller->SaveRoute.Directory,
                    FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("SmokeSave")))
                && !Controller->IsFailed();
        });
    Add(TEXT("Close initial guide with the production mapped Back input"),
        [this]() { if (Controller->IsBookOpen()) Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen() && !Controller->IsPlanning(); });

    FString Producer;
    bool Resume = false;
    const TCHAR* Cursor = FCommandLine::Get();
    FString Token;
    const FString ResumeName(TEXT("-HomesteadGeneratedResumeFrom"));
    while (FParse::Token(Cursor, Token, false))
    {
        if (!Token.StartsWith(ResumeName, ESearchCase::IgnoreCase)) continue;
        if (Resume || !Token.StartsWith(ResumeName + TEXT("="), ESearchCase::IgnoreCase))
        { Finish(false, TEXT("Supply exactly one -HomesteadGeneratedResumeFrom=<absolute producer output>.")); return; }
        Resume = true;
        Producer = Token.Mid(ResumeName.Len() + 1);
        if (Producer.StartsWith(TEXT("\"")) && Producer.EndsWith(TEXT("\"")) && Producer.Len() >= 2)
            Producer = Producer.Mid(1, Producer.Len() - 2);
        if (Producer.IsEmpty() || Producer.Contains(TEXT("\"")))
        { Finish(false, TEXT("Generated resume requires one nonempty correctly quoted producer path.")); return; }
    }
    if (Resume)
    {
        const auto Admitted = MakeShared<bool>(false);
        Add(TEXT("Copy only the pinned generated producer proof/save into a fresh isolated manual slot"),
            [this, Fixture, Producer, Admitted]()
            {
                TArray<uint8> Bytes;
                const FString Destination = Controller->SavePath(TEXT("Homestead_Manual"));
                if (!ReadProof(Producer, *Fixture, Bytes) || IFileManager::Get().FileExists(*Destination)
                    || !IFileManager::Get().MakeDirectory(*Controller->SaveRoute.Directory, true)
                    || !HomesteadNativeMenuProof::OrdinaryLocalPath(Controller->SaveRoute.Directory, true)
                    || !FFileHelper::SaveArrayToFile(Bytes, *Destination)
                    || !HomesteadNativeMenuProof::OrdinaryLocalPath(Destination, false)) return;
                TArray<uint8> Copied;
                const auto* Save = Controller->ReadSave(Destination);
                *Admitted = FFileHelper::LoadFileToArray(Copied, *Destination)
                    && FMD5::HashBytes(Copied.GetData(), Copied.Num()) == Fixture->Fingerprint
                    && Save && Save->IsCurrentVersion() && Save->WorldId == Fixture->WorldId
                    && Save->SimulationData == Fixture->SimulationText;
            }, [Admitted]() { return *Admitted; });
        const auto Loaded = MakeShared<bool>(false);
        const auto LoadsBefore = MakeShared<uint32>(0);
        Add(TEXT("Separate process loads the actual producer manual save through mapped F9"),
            [this, LoadsBefore]()
            {
                *LoadsBefore = Controller->TestQuickLoads;
                Tap(EKeys::F9);
            },
            [this, LoadsBefore]() { return Controller->TestQuickLoads == *LoadsBefore + 1
                && !Controller->ToastIsError(); });
        Add(TEXT("Separate process verifies the producer descriptor and durable site"),
            [this, Fixture, Loaded]() { *Loaded = Controller->State().world.seed == Fixture->World.seed
                && Controller->State().world.generationVersion == Fixture->World.generationVersion; },
            [this, Fixture, Loaded]()
            {
                const bool Good = *Loaded && !Controller->ToastIsError()
                    && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
                    && Controller->WorldId == Fixture->WorldId && SitePersists(Controller->Simulation(), *Fixture);
                if (Good) Results.Add(FString::Printf(TEXT("GENERATED_RESUME producer_pid=%u consumer_pid=%u save_md5=%s; controlled producer site, not a new untouched/walking claim"),
                    Fixture->ProducerProcess, FPlatformProcess::GetCurrentProcessId(), *Fixture->Fingerprint));
                return Good;
            }, 1.0f);
        Add(TEXT("Reloaded active tree batches and capsules match authoritative active trees"),
            []() {}, [this, ActiveBatchSnapshot]()
            {
                int32 Components = 0, Instances = 0, Collisions = 0, Expected = 0;
                for (const auto& Node : Controller->State().resources)
                    Expected += Node.kind == ResourceKind::ForestTree && !Node.cleared ? 1 : 0;
                return !ActiveBatchSnapshot(Components, Instances, Collisions).IsEmpty()
                    && Components > 0 && Components <= 3
                    && Instances == Expected && Collisions == Expected;
            });
        Add(TEXT("Reloaded regional reach rematerializes with the same nonblocking policy"),
            []() {}, [this]()
            {
                if (!Controller->Landscape
                    || Controller->Landscape->RenderedRegionalReachKey != TEXT("0,-1>1,0")
                    || Controller->Landscape->RegionalWaterMeshes.IsEmpty())
                    return false;
                for (const auto& Entry : Controller->Landscape->RegionalWaterMeshes)
                {
                    const auto* Water = Entry.Value.Get();
                    if (!Water || !Water->IsRegistered() || Water->IsQueryCollisionEnabled()
                        || Water->GetGenerateOverlapEvents() || Water->CanEverAffectNavigation())
                        return false;
                }
                return true;
            }, 30.0f);
        Capture(TEXT("generated-reloaded"));
        Add(TEXT("External generated producer save remains byte-identical"),
            []() {}, [Producer, Fixture]()
            {
                TArray<uint8> Bytes;
                return FFileHelper::LoadFileToArray(Bytes,
                    *FPaths::Combine(Producer, TEXT("generated-woodland-fixture.sav")))
                    && FMD5::HashBytes(Bytes.GetData(), Bytes.Num()) == Fixture->Fingerprint;
            });
        return;
    }

    if (!ChooseSite(Controller->Simulation(), *Fixture))
    { Finish(false, TEXT("No bounded mature-tree worksite with an adjacent low-relief garden cell exists for this generated descriptor; no reroll.")); return; }
    if (!ChooseWalk(Controller->Simulation(), *Fixture))
    { Finish(false, TEXT("No naturally clear bounded old-edge/chunk-seam walking corridor exists for this generated descriptor; no reroll.")); return; }
    const Point Build = CellCenter(Fixture->BuildX, Fixture->BuildY);
    const Point Garden = CellCenter(Fixture->PlotX, Fixture->PlotY);
    Add(TEXT("Untouched generated start has no edits, structures, plots or supplied tools"),
        []() {},
        [this]()
        {
            const bool Good = Controller->State().world.generationVersion == Generation::WorldGenerationVersion
                && Controller->State().resourceEdits.empty() && Controller->State().structures.empty()
                && Controller->State().plots.empty() && Controller->Simulation().Count(Item::Hatchet) == 0;
            if (!Good) Results.Add(FString::Printf(
                TEXT("UNTOUCHED_DIAGNOSTIC state_version=%u expected=%u edits=%d structures=%d plots=%d hatchets=%d"),
                Controller->State().world.generationVersion, Generation::WorldGenerationVersion,
                Controller->State().resourceEdits.size(), Controller->State().structures.size(),
                Controller->State().plots.size(), Controller->Simulation().Count(Item::Hatchet)));
            return Good;
        });
    Capture(TEXT("generated-untouched"));
    Add(TEXT("Render one coherent canonical regional reach without blocking traversal"),
        []() {},
        [this]()
        {
            if (!Controller->Landscape
                || Controller->Landscape->RegionalDescriptors.CachedRegionCount() < 1
                || Controller->Landscape->RenderedRegionalReachKey != TEXT("0,-1>1,0")
                || Controller->Landscape->RegionalWaterMeshes.IsEmpty()
                || Controller->Landscape->RenderedRegionalReachReferences
                    != Controller->Landscape->RegionalWaterMeshes.Num())
                return false;
            for (const auto& Entry : Controller->Landscape->RegionalWaterMeshes)
            {
                const auto* Water = Entry.Value.Get();
                if (!Water || !Water->IsRegistered() || Water->IsQueryCollisionEnabled()
                    || Water->GetGenerateOverlapEvents() || Water->CanEverAffectNavigation()
                    || !Water->ComponentHasTag(TEXT("GeneratedRegionalWater")))
                    return false;
            }
            return true;
        }, 30.0f);
    Add(TEXT("CONTROLLED frame the actual rendered regional reach"),
        [this]()
        {
            if (!Controller->Landscape || Controller->Landscape->RegionalWaterMeshes.IsEmpty())
                return;
            const auto* Water = Controller->Landscape->RegionalWaterMeshes.CreateConstIterator().Value().Get();
            const FVector Center = Water->Bounds.Origin;
            const FVector2D Viewpoint(Center.X - 450.0, Center.Y - 300.0);
            Teleport({Viewpoint.X, Viewpoint.Y});
            Controller->SetControlRotation(FRotator(-22.0,
                FMath::RadiansToDegrees(FMath::Atan2(
                    Center.Y - Viewpoint.Y, Center.X - Viewpoint.X)), 0));
        },
        [this]()
        {
            return Controller->Landscape
                && !Controller->Landscape->RegionalWaterMeshes.IsEmpty()
                && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0;
        }, 1.0f);
    Capture(TEXT("generated-regional-water"));
    Add(TEXT("CONTROLLED initial supply: one hatchet only; no world outcomes fabricated"),
        [this]()
        {
            Homestead::Simulation Supplied = Controller->Simulation();
            // Only supply inventory/layout on a copy; current-version deserialization validates it.
            auto& Stock = const_cast<Homestead::State&>(Supplied.GetState());
            Stock.inventory[static_cast<int32>(Item::Hatchet)] = 1;
            Stock.inventoryLayout.push_back({Stock.nextGroupId++, Item::Hatchet, 1, 0});
            const auto Result = Controller->Sim.Deserialize(Supplied.Serialize());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
            Results.Add(TEXT("CONTROLLED fixture supplies one hatchet; no time warp or fabricated felling/build/plot; all subsequent outcomes use mapped inputs."));
        },
        [this]() { return Controller->State().resourceEdits.empty() && Controller->State().structures.empty()
            && Controller->State().plots.empty() && Controller->Simulation().Count(Item::Hatchet) == 1
            && Controller->Simulation().UsedCapacity() <= InventoryCapacity; });

    for (const auto& Planned : Fixture->Clear)
    {
        const auto Key = Planned.key;
        const FString KeyText = FString::Printf(TEXT("%d,%d,%u"), Key.chunk.x, Key.chunk.y, Key.localId);
        const bool Mature = Planned.kind == ResourceKind::ForestTree;
        const bool Primary = Key == Fixture->SiteTree;
        const auto BranchBefore = MakeShared<int32>(0), FiberBefore = MakeShared<int32>(0);
        const auto Positioned = MakeShared<bool>(false);
        const auto ActiveBefore = MakeShared<FString>();
        const auto ActiveInstancesBefore = MakeShared<int32>(0);
        const auto ActivePointersBefore =
            MakeShared<TSet<const UHierarchicalInstancedStaticMeshComponent*>>();
        Add(FString::Printf(TEXT("CONTROLLED worksite approach to generated key (%d,%d,%u), resolving current handle"),
            Key.chunk.x, Key.chunk.y, Key.localId),
            [this, Key, Positioned]()
            {
                ResourceNode Node;
                if (!Controller->Simulation().ResolveGeneratedResource(Key, Node)
                    || !Controller->PrepareWorldAt(Node.position)) return;
                const Point Offsets[] = {{0, 0}, {-150, 0}, {150, 0}, {0, -150}, {0, 150}};
                for (const Point Offset : Offsets)
                {
                    const Point Position{Node.position.x + Offset.x, Node.position.y + Offset.y};
                    ResourceNode Current;
                    if (!Controller->Simulation().ResolveGeneratedResource(Key, Current) || Current.id <= 0
                        || Controller->Simulation().FindNearestResource(Position, 280) != Current.id) continue;
                    bool Safe = true;
                    for (const auto& Other : Controller->State().resources)
                        if (!Other.cleared && (Other.kind == ResourceKind::ForestTree || Other.kind == ResourceKind::Sapling)
                            && DistanceSquared(Other.position, Position) < FMath::Square(110.0)) Safe = false;
                    if (!Safe) continue;
                    Teleport(Position);
                    Controller->SetControlRotation(FRotator(-15, FMath::RadiansToDegrees(
                        FMath::Atan2(-Offset.y, -Offset.x)), 0));
                    *Positioned = true;
                    break;
                }
            },
            [this, Key, Positioned]()
            {
                ResourceNode Node;
                return *Positioned && Controller->Simulation().ResolveGeneratedResource(Key, Node)
                    && Node.id > 0 && !Node.cleared && Controller->IsResourceFocused(Node.id);
            }, 0.8f);
        if (Mature)
            Add(TEXT("Pin active tree batch/collision state before mapped felling"),
                [this, ActiveBatchSnapshot, ActiveBefore, ActiveInstancesBefore, ActivePointersBefore]()
                {
                    int32 Components = 0, Collisions = 0;
                    *ActiveBefore = ActiveBatchSnapshot(Components, *ActiveInstancesBefore, Collisions);
                    for (const auto& Entry : Controller->Landscape->ActiveTreeBatches)
                        ActivePointersBefore->Add(Entry.Value.Get());
                },
                [this, KeyText, ActiveBefore, ActiveInstancesBefore]()
                {
                    return !ActiveBefore->IsEmpty() && *ActiveInstancesBefore > 0
                        && Controller->Landscape->ActiveTreeInstances.Contains(KeyText)
                        && Controller->Landscape->ActiveTreeCollisions.Contains(KeyText);
                });
        Add(Primary ? TEXT("Fell mature build-site tree through production primary A")
                    : TEXT("Clear actual worksite obstruction through production secondary X"),
            [this, Primary, BranchBefore, FiberBefore]()
            {
                *BranchBefore = Controller->Simulation().Count(Item::Branch);
                *FiberBefore = Controller->Simulation().Count(Item::Fiber);
                Tap(Primary ? EKeys::Gamepad_FaceButton_Bottom : EKeys::Gamepad_FaceButton_Left);
            },
            [this, Key, KeyText, Mature, BranchBefore, FiberBefore, ActiveBatchSnapshot,
                ActiveInstancesBefore, ActivePointersBefore]()
            {
                ResourceNode Node;
                if (!Controller->Simulation().ResolveGeneratedResource(Key, Node) || !Node.cleared
                    || Node.readyAtHour != 0 || Controller->ToastIsError()
                    || Controller->Simulation().UsedCapacity() > InventoryCapacity) return false;
                if (!Mature) return true;
                int32 Components = 0, Instances = 0, Collisions = 0;
                const FString After = ActiveBatchSnapshot(Components, Instances, Collisions);
                bool OldPointersRemoved = true;
                for (const auto& Entry : Controller->Landscape->ActiveTreeBatches)
                    OldPointersRemoved &= !ActivePointersBefore->Contains(Entry.Value.Get());
                return ClearedKey(Controller->Simulation(), Key, true)
                        && !After.IsEmpty() && Instances == *ActiveInstancesBefore - 1
                        && Collisions == Instances && Components > 0 && Components <= 3
                        && !Controller->Landscape->ActiveTreeInstances.Contains(KeyText)
                        && !Controller->Landscape->ActiveTreeCollisions.Contains(KeyText)
                        && OldPointersRemoved
                        && Controller->Simulation().Count(Item::Branch) == *BranchBefore + 8
                        && Controller->Simulation().Count(Item::Fiber) == *FiberBefore + 2;
            }, 0.8f);
    }
    Add(TEXT("CONTROLLED post-felling supply: digging/watering tools, build stones, roots, seed and water; retain earned wood/fiber"),
        [this]()
        {
            Homestead::Simulation Supplied = Controller->Simulation();
            auto& Stock = const_cast<Homestead::State&>(Supplied.GetState());
            const TPair<Item, int32> Minimums[] = {{Item::DiggingStick, 1}, {Item::WateringCan, 1},
                {Item::Branch, 8}, {Item::Stone, 6}, {Item::Roots, 2}, {Item::Seeds, 1}, {Item::Water, 1}};
            for (const auto& Supply : Minimums)
            {
                const int32 Index = static_cast<int32>(Supply.Key);
                const int32 Added = FMath::Max(0, Supply.Value - Stock.inventory[Index]);
                if (!Added) continue;
                Stock.inventory[Index] += Added;
                Stock.inventoryLayout.push_back({Stock.nextGroupId++, Supply.Key, Added, 0});
            }
            const auto Result = Controller->Sim.Deserialize(Supplied.Serialize());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
            Results.Add(TEXT("CONTROLLED post-felling materials supplied; earned tree rewards retained and later construction/cooking use production authority."));
        },
        [this]() { return Controller->Simulation().Count(Item::Branch) >= 8
            && Controller->Simulation().Count(Item::Stone) >= 6
            && Controller->Simulation().Count(Item::DiggingStick) == 1
            && Controller->Simulation().Count(Item::WateringCan) == 1
            && Controller->Simulation().UsedCapacity() <= InventoryCapacity; });
    QueuePlace(Piece::Foundation, Fixture->BuildX, Fixture->BuildY);
    Add(TEXT("CONTROLLED teleport to the cleared tree plot approach; no ordinary-travel claim"),
        [this, Garden]()
        {
            Teleport({Garden.x - 190, Garden.y});
            Controller->GetPawn()->SetActorRotation(FRotator::ZeroRotator);
            Controller->SetControlRotation(FRotator(-20, 0, 0));
        },
        [this]() { return Controller->Focus == AHomesteadController::EFocus::None; }, 0.8f);
    Add(TEXT("Till the actually felled garden cell through mapped X"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
        [this, Fixture]() { const auto* Plot = FixturePlot(Controller->State(), *Fixture);
            return Plot && !Plot->planted && !Controller->ToastIsError(); });
    Add(TEXT("CONTROLLED short plot approach"),
        [this, Garden]() { Teleport(Garden); },
        [this]() { return Controller->Focus == AHomesteadController::EFocus::Plot; }, 0.8f);
    Add(TEXT("Plant roots through mapped A using one disclosed seed"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Fixture]() { const auto* Plot = FixturePlot(Controller->State(), *Fixture);
            return Plot && Plot->planted && Plot->kind == CropKind::Roots && !Controller->ToastIsError(); });
    Add(TEXT("Water the actual planted plot through mapped A using disclosed water"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Fixture]() { const auto* Plot = FixturePlot(Controller->State(), *Fixture);
            return Plot && Plot->moisture > 0.99 && Controller->Simulation().Count(Item::Water) == 0
                && !Controller->ToastIsError(); }, 0.8f);
    QueuePlace(Piece::Fire, Fixture->BuildX, Fixture->BuildY);
    Add(TEXT("CONTROLLED approach to the player-built cookfire"),
        [this, Build]() { Teleport({Build.x - 200, Build.y}); },
        [this]() { return Controller->Focus == AHomesteadController::EFocus::Fire; }, 0.8f);
    Add(TEXT("Fuel the actual cookfire with mapped X"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Left); },
        [this]() { return !Controller->ToastIsError() && Controller->Simulation().IsNearFire(Controller->PlayerPoint()); });
    QueueCraft(Recipe::RoastedRoots);
    Add(TEXT("Frame the genuinely cleared build and watered plot with the gameplay camera"),
        [this, Garden]()
        {
            Teleport(Garden);
            Controller->SetControlRotation(FRotator(-25, -90, 0));
        },
        [this, Fixture]() { return SitePersists(Controller->Simulation(), *Fixture); }, 1.0f);
    Capture(TEXT("generated-cleared-site"));
    Add(TEXT("Active mature trees use bounded exact-mesh batches and per-key capsules"),
        [Fixture, ActiveBatchSnapshot]()
        {
            Fixture->ActiveBatchSnapshot = ActiveBatchSnapshot(Fixture->ActiveBatchComponents,
                Fixture->ActiveBatchInstances, Fixture->ActiveCollisionCapsules);
        },
        [Fixture]()
        {
            return !Fixture->ActiveBatchSnapshot.IsEmpty()
                && Fixture->ActiveBatchComponents > 0 && Fixture->ActiveBatchComponents <= 3
                && Fixture->ActiveBatchInstances > Fixture->ActiveBatchComponents
                && Fixture->ActiveCollisionCapsules == Fixture->ActiveBatchInstances;
        });
    Add(TEXT("Outer mature trees use bounded exact-mesh batches before travel"),
        [Fixture, OuterBatchSnapshot]()
        {
            Fixture->OuterBatchSnapshot = OuterBatchSnapshot(
                Fixture->OuterBatchComponents, Fixture->OuterBatchInstances);
        },
        [Fixture]()
        {
            return !Fixture->OuterBatchSnapshot.IsEmpty()
                && Fixture->OuterBatchComponents > 0 && Fixture->OuterBatchComponents <= 3
                && Fixture->OuterBatchInstances > Fixture->OuterBatchComponents;
        });
    Add(TEXT("CONTROLLED teleport to the surveyed negative-coordinate walk start, before the old -4000 board edge"),
        [this, Fixture]()
        {
            Fixture->EditCount = Controller->State().resourceEdits.size();
            Teleport(Fixture->WalkStart);
            Controller->GetPawn()->SetActorRotation(FRotator(0, 180, 0));
            Controller->SetControlRotation(FRotator(-15, 180, 0));
        },
        [this, Fixture]() { return FMath::Abs(Controller->PlayerPoint().x + 3750) < 20
            && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
            && Controller->State().resourceEdits.size() == Fixture->EditCount; }, 1.0f);
    Add(TEXT("Naturally walk across old -4000 edge and -4800 generated seam using held mapped left-stick input"),
        [this, Fixture]()
        {
            Fixture->WalkBefore = Fixture->Previous = Controller->GetPawn()->GetActorLocation();
        },
        [this, Fixture]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            Fixture->WalkAfter = Controller->GetPawn()->GetActorLocation();
            Results.Add(FString::Printf(TEXT("GENERATED_WALK mapped_axis_only from=%s to=%s old_edge=%d seam=%d grounded_continuous=%d recoveries=%u"),
                *Fixture->WalkBefore.ToString(), *Fixture->WalkAfter.ToString(), Fixture->CrossedOldBoundary,
                Fixture->CrossedChunkSeam, Fixture->ContinuousGround, Controller->WorldRecoveryCount()));
            return Fixture->WalkBefore.X > -4000 && Fixture->WalkAfter.X < -4900
                && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
                && Fixture->WalkAfter.X > -5550 && FMath::Abs(Fixture->WalkAfter.Y - Fixture->WalkStart.y) < 70
                && Fixture->CrossedOldBoundary && Fixture->CrossedChunkSeam && Fixture->ContinuousGround
                && Controller->State().activeChunk.x == -3
                && Controller->State().resourceEdits.size() == Fixture->EditCount
                && ClearedKey(Controller->Simulation(), Fixture->SiteTree, false);
        }, 8.5f);
    Steps.Last().Repeat = [this, Fixture]()
    {
        const auto Position = Controller->GetPawn()->GetActorLocation();
        const auto* Character = Cast<ACharacter>(Controller->GetPawn());
        const double AboveGround = Position.Z - Controller->GroundHeight(Position.X, Position.Y);
        Fixture->ContinuousGround &= Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
            && Character && Character->GetCharacterMovement()->IsMovingOnGround()
            && AboveGround > 60 && AboveGround < 130 && FVector::Dist(Position, Fixture->Previous) < 220;
        Fixture->CrossedOldBoundary |= Fixture->Previous.X >= -4000 && Position.X < -4000;
        Fixture->CrossedChunkSeam |= Fixture->Previous.X >= -4800 && Position.X < -4800;
        Fixture->Previous = Position;
        Axis(EKeys::Gamepad_LeftY, 1);
    };
    Add(TEXT("Release movement after the actual boundary traversal"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [this]() { return Controller->GetPawn()->GetVelocity().Size2D() < 5; }, 0.8f);
    Capture(TEXT("generated-boundary"));
    Add(TEXT("CONTROLLED return teleport prepares collision, then resolves saved tree KEYS instead of old handles"),
        [this, Garden]()
        {
            Teleport(Garden);
            Controller->SetControlRotation(FRotator(-25, -90, 0));
        },
        [this, Fixture, ActiveBatchSnapshot]()
        {
            int32 Components = 0, Instances = 0, Collisions = 0;
            return SitePersists(Controller->Simulation(), *Fixture)
                && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
                && Controller->State().resourceEdits.size() == Fixture->EditCount
                && ActiveBatchSnapshot(Components, Instances, Collisions)
                    == Fixture->ActiveBatchSnapshot
                && Components == Fixture->ActiveBatchComponents
                && Instances == Fixture->ActiveBatchInstances
                && Collisions == Fixture->ActiveCollisionCapsules;
        }, 1.0f);
    const auto ActiveLayoutStable = MakeShared<bool>(false);
    Add(TEXT("Same active layout preserves batch and capsule component identities"),
        [this, Fixture, ActiveBatchSnapshot, ActiveLayoutStable]()
        {
            TSet<const UHierarchicalInstancedStaticMeshComponent*> Batches;
            TSet<const UCapsuleComponent*> Collisions;
            for (const auto& Entry : Controller->Landscape->ActiveTreeBatches)
                Batches.Add(Entry.Value.Get());
            for (const auto& Entry : Controller->Landscape->ActiveTreeCollisions)
                Collisions.Add(Entry.Value.Get());
            if (!Controller->Landscape->Refresh(Controller->Simulation())) return;
            int32 Components = 0, Instances = 0, CollisionCount = 0;
            bool Stable = ActiveBatchSnapshot(Components, Instances, CollisionCount)
                == Fixture->ActiveBatchSnapshot;
            for (const auto& Entry : Controller->Landscape->ActiveTreeBatches)
                Stable &= Batches.Contains(Entry.Value.Get());
            for (const auto& Entry : Controller->Landscape->ActiveTreeCollisions)
                Stable &= Collisions.Contains(Entry.Value.Get());
            *ActiveLayoutStable = Stable;
        },
        [ActiveLayoutStable]() { return *ActiveLayoutStable; });
    const auto ActiveRollbackValid = MakeShared<bool>(false);
    Add(TEXT("Failed destination refresh invalidates active signature before prior-world recovery"),
        [this, Fixture, ActiveBatchSnapshot, ActiveRollbackValid]()
        {
            Homestead::Simulation FailedDestination = Controller->Simulation();
            auto& FailedState = const_cast<Homestead::State&>(FailedDestination.GetState());
            FailedState.world.seed ^= 0x517cc1b727220a95ULL;
            auto Tree = FailedState.resources.end();
            for (auto It = FailedState.resources.begin(); It != FailedState.resources.end(); ++It)
                if (It->kind == ResourceKind::ForestTree && !It->cleared)
                {
                    Tree = It;
                    break;
                }
            if (Tree == FailedState.resources.end()) return;
            Tree->key.localId = MAX_uint32;
            if (Controller->Landscape->Refresh(FailedDestination)
                || !Controller->Landscape->ActiveTreeBatches.IsEmpty()
                || !Controller->Landscape->ActiveTreeCollisions.IsEmpty()
                || !Controller->Landscape->ActiveTreeInstances.IsEmpty()
                || !Controller->Landscape->ActiveTreeLayoutSignature.IsEmpty())
                return;
            if (!Controller->Landscape->Refresh(Controller->Simulation())) return;
            int32 Components = 0, Instances = 0, Collisions = 0;
            *ActiveRollbackValid = ActiveBatchSnapshot(Components, Instances, Collisions)
                    == Fixture->ActiveBatchSnapshot
                && Components == Fixture->ActiveBatchComponents
                && Instances == Fixture->ActiveBatchInstances
                && Collisions == Fixture->ActiveCollisionCapsules;
        },
        [this, ActiveRollbackValid]()
        {
            return *ActiveRollbackValid && Controller->IsWorldReady()
                && Controller->WorldRecoveryCount() == 0;
        });
    const auto BatchLifecycleValid = MakeShared<bool>(false);
    Add(TEXT("Outer batches ignore renewable timers, omit cleared trees and rebuild without accumulation"),
        [this, Fixture, ActiveBatchSnapshot, OuterBatchSnapshot, BatchLifecycleValid]()
        {
            int32 ReturnedComponents = 0, ReturnedInstances = 0;
            const FString Returned = OuterBatchSnapshot(ReturnedComponents, ReturnedInstances);
            if (Returned != Fixture->OuterBatchSnapshot
                || ReturnedComponents != Fixture->OuterBatchComponents
                || ReturnedInstances != Fixture->OuterBatchInstances)
                return;

            Homestead::Simulation Shifted = Controller->Simulation();
            const Point ShiftedCenter{
                (Fixture->SiteTree.chunk.x + 2.5) * Generation::ChunkSizeCm,
                (Fixture->SiteTree.chunk.y + 0.5) * Generation::ChunkSizeCm};
            if (!Shifted.SetActiveWorldRegion(ShiftedCenter)
                || !Controller->Landscape->Refresh(Shifted))
                return;
            const FString ClearedKey = FString::Printf(TEXT("%d,%d,%u"),
                Fixture->SiteTree.chunk.x, Fixture->SiteTree.chunk.y, Fixture->SiteTree.localId);
            if (Controller->Landscape->OuterTreeInstances.Contains(ClearedKey)
                || FMath::Abs(Shifted.GetState().activeChunk.x - Fixture->SiteTree.chunk.x) != 2)
                return;
            int32 ShiftedComponents = 0, ShiftedInstances = 0;
            const FString ShiftedSnapshot = OuterBatchSnapshot(ShiftedComponents, ShiftedInstances);
            if (ShiftedSnapshot.IsEmpty()) return;
            TSet<const UHierarchicalInstancedStaticMeshComponent*> ShiftedPointers;
            for (const auto& Entry : Controller->Landscape->OuterTreeBatches)
                ShiftedPointers.Add(Entry.Value.Get());
            if (!Controller->Landscape->Refresh(Shifted)) return;
            int32 RepeatedComponents = 0, RepeatedInstances = 0;
            if (OuterBatchSnapshot(RepeatedComponents, RepeatedInstances) != ShiftedSnapshot
                || RepeatedComponents != ShiftedComponents || RepeatedInstances != ShiftedInstances)
                return;
            for (const auto& Entry : Controller->Landscape->OuterTreeBatches)
                if (!ShiftedPointers.Contains(Entry.Value.Get())) return;

            Homestead::Simulation Renewed = Shifted;
            auto& RenewedState = const_cast<Homestead::State&>(Renewed.GetState());
            auto Renewable = RenewedState.resources.end();
            for (auto It = RenewedState.resources.begin(); It != RenewedState.resources.end(); ++It)
                if (It->kind != ResourceKind::ForestTree && It->kind != ResourceKind::Sapling
                    && !It->cleared)
                {
                    Renewable = It;
                    break;
                }
            if (Renewable == RenewedState.resources.end()) return;
            Renewable->readyAtHour = RenewedState.hour + 1.0;
            auto RenewableEdit = std::lower_bound(RenewedState.resourceEdits.begin(),
                RenewedState.resourceEdits.end(), Renewable->key,
                [](const ResourceEdit& Value, const Generation::GeneratedEntityKey& Key)
                {
                    return Value.key < Key;
                });
            if (RenewableEdit != RenewedState.resourceEdits.end()
                && RenewableEdit->key == Renewable->key)
                *RenewableEdit = {Renewable->key, false, Renewable->readyAtHour};
            else
                RenewedState.resourceEdits.insert(
                    RenewableEdit, {Renewable->key, false, Renewable->readyAtHour});
            if (!Controller->Landscape->Refresh(Renewed)) return;
            int32 RenewedComponents = 0, RenewedInstances = 0;
            if (OuterBatchSnapshot(RenewedComponents, RenewedInstances) != ShiftedSnapshot
                || RenewedComponents != ShiftedComponents || RenewedInstances != ShiftedInstances)
                return;
            for (const auto& Entry : Controller->Landscape->OuterTreeBatches)
                if (!ShiftedPointers.Contains(Entry.Value.Get())) return;

            FString OuterKeyText;
            for (const auto& Entry : Controller->Landscape->OuterTreeInstances)
            {
                OuterKeyText = Entry.Key;
                break;
            }
            TArray<FString> KeyParts;
            OuterKeyText.ParseIntoArray(KeyParts, TEXT(","));
            if (KeyParts.Num() != 3) return;
            const Generation::GeneratedEntityKey OuterKey{
                {FCString::Atoi(*KeyParts[0]), FCString::Atoi(*KeyParts[1])},
                static_cast<uint32>(FCString::Atoi(*KeyParts[2]))};
            Homestead::Simulation ClearedOuter = Renewed;
            auto& ClearedState = const_cast<Homestead::State&>(ClearedOuter.GetState());
            auto OuterEdit = std::lower_bound(ClearedState.resourceEdits.begin(),
                ClearedState.resourceEdits.end(), OuterKey,
                [](const ResourceEdit& Value, const Generation::GeneratedEntityKey& Key)
                {
                    return Value.key < Key;
                });
            if (OuterEdit != ClearedState.resourceEdits.end() && OuterEdit->key == OuterKey)
                *OuterEdit = {OuterKey, true, 0.0};
            else
                ClearedState.resourceEdits.insert(OuterEdit, {OuterKey, true, 0.0});
            if (!Controller->Landscape->Refresh(ClearedOuter)
                || Controller->Landscape->OuterTreeInstances.Contains(OuterKeyText)
                || Controller->Landscape->OuterTreeInstances.Num() != ShiftedInstances - 1)
                return;
            int32 ClearedComponents = 0, ClearedInstances = 0;
            if (OuterBatchSnapshot(ClearedComponents, ClearedInstances).IsEmpty()
                || ClearedInstances != ShiftedInstances - 1)
                return;
            for (const auto& Entry : Controller->Landscape->OuterTreeBatches)
                if (ShiftedPointers.Contains(Entry.Value.Get())) return;

            TSet<const UHierarchicalInstancedStaticMeshComponent*> PreviousActiveBatches;
            TSet<const UCapsuleComponent*> PreviousActiveCollisions;
            for (const auto& Entry : Controller->Landscape->ActiveTreeBatches)
                PreviousActiveBatches.Add(Entry.Value.Get());
            for (const auto& Entry : Controller->Landscape->ActiveTreeCollisions)
                PreviousActiveCollisions.Add(Entry.Value.Get());
            Homestead::Simulation Alternate;
            if (!Alternate.NewGame(Controller->State().world.seed ^ 0x9e3779b97f4a7c15ULL)
                || !Alternate.SetActiveWorldRegion(ShiftedCenter)
                || !Controller->Landscape->Refresh(Alternate)
                || Controller->Landscape->Descriptor.seed != Alternate.GetState().world.seed)
                return;
            int32 AlternateActiveComponents = 0, AlternateActiveInstances = 0;
            int32 AlternateActiveCollisions = 0;
            if (ActiveBatchSnapshot(AlternateActiveComponents, AlternateActiveInstances,
                    AlternateActiveCollisions).IsEmpty()
                || AlternateActiveInstances != AlternateActiveCollisions)
                return;
            for (const auto& Entry : Controller->Landscape->ActiveTreeBatches)
                if (PreviousActiveBatches.Contains(Entry.Value.Get())) return;
            for (const auto& Entry : Controller->Landscape->ActiveTreeCollisions)
                if (PreviousActiveCollisions.Contains(Entry.Value.Get())) return;
            int32 AlternateComponents = 0, AlternateInstances = 0;
            if (OuterBatchSnapshot(AlternateComponents, AlternateInstances).IsEmpty()
                || AlternateComponents <= 0 || AlternateComponents > 3
                || AlternateInstances != Controller->Landscape->OuterTreeInstances.Num())
                return;

            if (!Controller->Landscape->Refresh(Controller->Simulation())) return;
            int32 RestoredComponents = 0, RestoredInstances = 0;
            int32 RestoredActiveComponents = 0, RestoredActiveInstances = 0;
            int32 RestoredActiveCollisions = 0;
            *BatchLifecycleValid = OuterBatchSnapshot(RestoredComponents, RestoredInstances)
                    == Fixture->OuterBatchSnapshot
                && RestoredComponents == Fixture->OuterBatchComponents
                && RestoredInstances == Fixture->OuterBatchInstances
                && ActiveBatchSnapshot(RestoredActiveComponents, RestoredActiveInstances,
                    RestoredActiveCollisions) == Fixture->ActiveBatchSnapshot
                && RestoredActiveComponents == Fixture->ActiveBatchComponents
                && RestoredActiveInstances == Fixture->ActiveBatchInstances
                && RestoredActiveCollisions == Fixture->ActiveCollisionCapsules
                && Controller->Landscape->Descriptor.generationVersion
                    == Controller->State().world.generationVersion;
        },
        [this, BatchLifecycleValid]()
        {
            return *BatchLifecycleValid && Controller->IsWorldReady()
                && Controller->WorldRecoveryCount() == 0;
        }, 1.0f);
    const auto Saved = MakeShared<bool>(false);
    const auto SavesBefore = MakeShared<uint32>(0);
    Add(TEXT("Save the real controlled worksite through mapped F5"),
        [this, SavesBefore]()
        {
            *SavesBefore = Controller->TestQuickSaves;
            Tap(EKeys::F5);
        },
        [this, SavesBefore]() { return Controller->TestQuickSaves == *SavesBefore + 1
            && !Controller->ToastIsError()
            && IFileManager::Get().FileExists(*Controller->SavePath(TEXT("Homestead_Manual"))); });
    Add(TEXT("Pin the completed current-version producer save bytes"),
        [this, Fixture, Saved]()
        {
            const FString Manual = Controller->SavePath(TEXT("Homestead_Manual"));
            const auto* Save = Controller->ReadSave(Manual);
            TArray<uint8> Bytes;
            if (!Save || !Save->IsCurrentVersion() || !FFileHelper::LoadFileToArray(Bytes, *Manual)) return;
            Fixture->WorldId = Save->WorldId;
            Fixture->SimulationText = Save->SimulationData;
            Fixture->ProducerProcess = FPlatformProcess::GetCurrentProcessId();
            Fixture->Fingerprint = FMD5::HashBytes(Bytes.GetData(), Bytes.Num());
            FString Json;
            const FString Output = HomesteadTestOutputDirectory();
            *Saved = FJsonSerializer::Serialize(ProofJson(*Fixture), TJsonWriterFactory<>::Create(&Json))
                && FFileHelper::SaveArrayToFile(Bytes, *FPaths::Combine(Output, TEXT("generated-woodland-fixture.sav")))
                && FFileHelper::SaveStringToFile(Json, *FPaths::Combine(Output, TEXT("generated-woodland-fixture.json")));
        },
        [this, Saved]() { return *Saved
            && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0; });
    Add(TEXT("CONTROLLED leave the saved site before local reload"),
        [this, Fixture]() { Teleport(Fixture->WalkStart); },
        [this]() { return Controller->PlayerPoint().x < -3500; }, 0.8f);
    const auto Reloaded = MakeShared<bool>(false);
    const auto LoadsBefore = MakeShared<uint32>(0);
    Add(TEXT("Reload the completed controlled save through mapped F9"),
        [this, LoadsBefore]()
        {
            *LoadsBefore = Controller->TestQuickLoads;
            Tap(EKeys::F9);
        },
        [this, LoadsBefore]() { return Controller->TestQuickLoads == *LoadsBefore + 1
            && !Controller->ToastIsError(); });
    Add(TEXT("Verify reloaded descriptor, permanent tree edit, construction and watered plot"),
        [this, Fixture, Reloaded]()
        {
            *Reloaded = Controller->State().world.seed == Fixture->World.seed
                && Controller->State().world.generationVersion == Fixture->World.generationVersion;
        },
        [this, Fixture, Reloaded]() { return *Reloaded && !Controller->ToastIsError()
            && Controller->IsWorldReady() && Controller->WorldRecoveryCount() == 0
            && Controller->WorldId == Fixture->WorldId && SitePersists(Controller->Simulation(), *Fixture); }, 1.0f);
    Capture(TEXT("generated-reloaded"));
}
