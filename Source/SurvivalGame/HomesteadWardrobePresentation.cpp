#include "HomesteadWardrobePresentation.h"
#include "HomesteadWardrobeSelection.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
bool Reject(FString& Error, const FString& Message)
{
    Error = Message;
    UE_LOG(LogTemp, Error, TEXT("Wardrobe presentation rejected: %s"), *Error);
    return false;
}

bool SameBind(const USkeletalMesh& Mesh, const USkeletalMesh& Reference)
{
    if (!Mesh.GetSkeleton() || Mesh.GetSkeleton() != Reference.GetSkeleton()) return false;
    const FReferenceSkeleton& Actual = Mesh.GetRefSkeleton();
    const FReferenceSkeleton& Expected = Reference.GetRefSkeleton();
    if (Actual.GetNum() != Expected.GetNum()) return false;
    for (int32 Index = 0; Index < Expected.GetNum(); ++Index)
    {
        if (Actual.GetBoneName(Index) != Expected.GetBoneName(Index)
            || Actual.GetParentIndex(Index) != Expected.GetParentIndex(Index)
            || !Actual.GetRefBonePose()[Index].Equals(Expected.GetRefBonePose()[Index], 0.01f))
            return false;
    }
    return true;
}

bool HasVector(const UMaterialInterface& Material, FName Name)
{
    TArray<FMaterialParameterInfo> Parameters;
    TArray<FGuid> Ids;
    Material.GetAllVectorParameterInfo(Parameters, Ids);
    return Parameters.ContainsByPredicate([Name](const FMaterialParameterInfo& P) { return P.Name == Name; });
}

bool HasScalar(const UMaterialInterface& Material, FName Name)
{
    TArray<FMaterialParameterInfo> Parameters;
    TArray<FGuid> Ids;
    Material.GetAllScalarParameterInfo(Parameters, Ids);
    return Parameters.ContainsByPredicate([Name](const FMaterialParameterInfo& P) { return P.Name == Name; });
}

bool IsHair(FName Name)
{
    return Name.ToString().StartsWith(TEXT("M_Heroine_Hair_")) || Name == TEXT("M_Heroine_Eyebrows");
}

bool IsNeutralHair(FName Name)
{
    return Name == TEXT("M_Heroine_Hair_long01_Neutral") || Name == TEXT("M_Heroine_Hair_bob01_Neutral");
}

bool IsDyeLinen(FName Name)
{
    return Name == TEXT("M_Heroine_MossLinen") || Name == TEXT("M_Heroine_ApronLinen");
}

TArray<FName> ExpectedMaterials(int32 Definition, int32 Hair)
{
    switch (static_cast<Homestead::WearableDefinition>(Definition))
    {
    case Homestead::WearableDefinition::LinenTunic:
        return {TEXT("M_Heroine_MossLinen"), TEXT("M_Heroine_LinenTrim"),
            TEXT("M_Heroine_ChestnutLeather"), TEXT("M_Heroine_Brass")};
    case Homestead::WearableDefinition::LinenApron:
        return {TEXT("M_Heroine_ApronTrim"), TEXT("M_Heroine_ApronLinen")};
    case Homestead::WearableDefinition::LeatherShoes:
        return {TEXT("M_Heroine_LeatherShoes")};
    case Homestead::WearableDefinition::WovenFootwraps:
        return {TEXT("M_Modular_FootwrapCloth"), TEXT("M_Modular_FootwrapBinding")};
    default:
        if (Definition != INDEX_NONE) return {};
        const FName Hairs[] = {TEXT("M_Heroine_Hair_long01"), TEXT("M_Heroine_Hair_bob01"),
            TEXT("M_Heroine_Hair_ponytail01")};
        return {TEXT("M_Heroine_Skin"), TEXT("M_Modular_BaseBra"), TEXT("M_Modular_BaseBriefs"), Hairs[Hair],
            TEXT("M_Heroine_LightEyes"), TEXT("M_Heroine_Eyebrows"), TEXT("M_Heroine_Eyelashes"),
            TEXT("M_Heroine_Teeth"), TEXT("M_Heroine_Tongue")};
    }
}

bool ValidateMesh(USkeletalMesh& Mesh, const USkeletalMesh& Reference,
    int32 Definition, const FHomesteadAppearance& Look, FString& Error)
{
    if (!SameBind(Mesh, Reference))
        return Reject(Error, FString::Printf(TEXT("%s does not share the admitted original bind."), *Mesh.GetName()));
    const auto& Materials = Mesh.GetMaterials();
    const TArray<FName> Expected = ExpectedMaterials(Definition, Look.HairStyle);
    if (Materials.Num() != Expected.Num())
        return Reject(Error, FString::Printf(TEXT("%s has an unexpected material slot count."), *Mesh.GetName()));
    for (int32 Index = 0; Index < Materials.Num(); ++Index)
    {
        const auto& Slot = Materials[Index];
        const FName Name = Slot.MaterialSlotName;
        UMaterialInterface* Surface = Slot.MaterialInterface;
        const bool NeutralHairSlot = IsNeutralHair(Name)
            && Name.ToString() == Expected[Index].ToString() + TEXT("_Neutral");
        if ((Name != Expected[Index] && !NeutralHairSlot) || !Surface
            || Surface->GetMaterial() == UMaterial::GetDefaultMaterial(MD_Surface))
            return Reject(Error, FString::Printf(TEXT("%s material %d is missing or has the wrong role."),
                *Mesh.GetName(), Index));
        if ((Name == TEXT("M_Heroine_Skin") || IsHair(Name) || IsDyeLinen(Name))
            && !HasVector(*Surface, TEXT("ColorTint")))
            return Reject(Error, FString::Printf(TEXT("%s needs ColorTint."), *Name.ToString()));
        if (Name == TEXT("M_Heroine_LightEyes")
            && (!HasVector(*Surface, TEXT("IrisColor")) || !HasScalar(*Surface, TEXT("IrisMix"))))
            return Reject(Error, TEXT("Eye material is missing iris-only controls."));
        const bool Masked = IsHair(Name) || Name == TEXT("M_Heroine_Eyelashes") || Name == TEXT("M_Heroine_LightEyes");
        if (Surface->GetBlendMode() != (Masked ? BLEND_Masked : BLEND_Opaque)
            || !Surface->GetShadingModels().HasShadingModel(MSM_DefaultLit))
            return Reject(Error, FString::Printf(TEXT("%s has incompatible opacity or shading."), *Name.ToString()));
    }
    const float Height = Mesh.GetBounds().BoxExtent.Z * 2.0f;
    if (!FMath::IsFinite(Height) || Height <= 0
        || (Definition == INDEX_NONE && (Height < 155 || Height > 175))
        || (Definition != INDEX_NONE && Height > 140))
        return Reject(Error, FString::Printf(TEXT("%s has invalid centimetre bounds."), *Mesh.GetName()));
    return true;
}

USkeletalMesh* LoadFit(const FString& Body, const FString& Name,
    USkeletalMesh& Reference, int32 Definition, const FHomesteadAppearance& Look, FString& Error)
{
    const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Characters/ModularClothing/%s/%s.%s"),
        *Body, *Name, *Name);
    USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Path);
    if (!Mesh)
    {
        Reject(Error, FString::Printf(TEXT("Required modular clothing is not installed: %s"), *Path));
        return nullptr;
    }
    return ValidateMesh(*Mesh, Reference, Definition, Look, Error) ? Mesh : nullptr;
}

bool PrepareSurface(UObject* Owner, USkeletalMesh* Mesh, const FHomesteadAppearance& Look,
    int32 Dye, FHomesteadEquipmentSurface& Out, FString& Error)
{
    Out.Mesh = Mesh;
    Out.Dye = Dye;
    for (const auto& Slot : Mesh->GetMaterials())
    {
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Slot.MaterialInterface, Owner);
        if (!Material) return Reject(Error, TEXT("Could not allocate a prepared wardrobe material."));
        const FName Name = Slot.MaterialSlotName;
        if (Name == TEXT("M_Heroine_Skin"))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::SkinTint(Look.SkinTone));
        else if (IsNeutralHair(Name))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::NeutralHairTint(Look.HairColor));
        else if (IsHair(Name))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::HairTint(Look.HairColor));
        else if (IsDyeLinen(Name))
            Material->SetVectorParameterValue(TEXT("ColorTint"), HomesteadLook::TunicTint(Dye));
        else if (Name == TEXT("M_Heroine_LightEyes"))
        {
            Material->SetVectorParameterValue(TEXT("IrisColor"), HomesteadLook::IrisColor(Look.EyeColor));
            Material->SetScalarParameterValue(TEXT("IrisMix"), Look.EyeColor == 0 ? 0.0f : 1.0f);
        }
        Out.Materials.Add(Material);
    }
    return true;
}
}

bool HomesteadWardrobePresentation::Prepare(UObject* Owner, const Homestead::State& State,
    const FHomesteadAppearance& Appearance, USkeletalMesh& Reference,
    FHomesteadEquipmentPresentation& Out, FString& Error)
{
    Out = {};
    Error.Reset();
    if (!IsInGameThread() || !Owner || !Appearance.IsValid())
        return Reject(Error, TEXT("Invalid appearance or non-game-thread wardrobe preparation."));
    Homestead::WardrobeSelection Selection;
    std::string SelectionError;
    if (!Homestead::SelectWardrobe(State, Appearance.BodyPreset, Appearance.HairStyle, Selection, SelectionError))
        return Reject(Error, UTF8_TO_TCHAR(SelectionError.c_str()));
    FHomesteadEquipmentPresentation Candidate;
    Candidate.Appearance = Appearance;
    const FString Body = UTF8_TO_TCHAR(Selection.body.c_str());
    USkeletalMesh* Base = LoadFit(Body, UTF8_TO_TCHAR(Selection.base.c_str()),
        Reference, INDEX_NONE, Appearance, Error);
    if (!Base || !PrepareSurface(Owner, Base, Appearance, 0, Candidate.Base, Error)) return false;
    TMap<int32, USkeletalMesh*> Fits;
    for (Homestead::WearableDefinition Definition : Selection.ownedDefinitions)
    {
        const FString Name = FString::Printf(TEXT("SK_Modular_%s_%s"), *Body,
            UTF8_TO_TCHAR(Homestead::GarmentAssetSuffix(Definition)));
        USkeletalMesh* Mesh = LoadFit(Body, Name, Reference, static_cast<int32>(Definition), Appearance, Error);
        if (!Mesh) return false;
        Fits.Add(static_cast<int32>(Definition), Mesh);
        Candidate.AdmittedMeshes.Add(Mesh);
    }
    for (const auto& Item : Selection.equipped)
    {
        FHomesteadEquipmentSurface Surface;
        Surface.WearableId = Item.id;
        Surface.Definition = static_cast<int32>(Item.definition);
        Surface.Slot = Item.definition == Homestead::WearableDefinition::LinenTunic
            ? static_cast<int32>(Homestead::EquipmentSlot::Torso)
            : Item.definition == Homestead::WearableDefinition::LinenApron
                ? static_cast<int32>(Homestead::EquipmentSlot::Apron)
                : static_cast<int32>(Homestead::EquipmentSlot::Feet);
        if (!PrepareSurface(Owner, Fits.FindChecked(Surface.Definition), Appearance, Item.dye, Surface, Error))
            return false;
        Candidate.Garments.Add(MoveTemp(Surface));
    }
    Candidate.Ready = true;
    Out = MoveTemp(Candidate);
    return true;
}

void HomesteadWardrobePresentation::ApplySurface(
    const FHomesteadEquipmentSurface& Surface, USkeletalMeshComponent& Component)
{
    if (Component.GetSkeletalMeshAsset() != Surface.Mesh)
    {
        Component.EmptyOverrideMaterials();
        Component.SetSkeletalMesh(Surface.Mesh, false);
    }
    for (int32 Index = 0; Index < Surface.Materials.Num(); ++Index)
        Component.SetMaterial(Index, Surface.Materials[Index]);
}
