// Places the sea cave (AHomesteadSeaCave) once with the estate scenery.
#include "HomesteadWorld.h"
#include "HomesteadSeaCaveScenery.h"

#include "Engine/World.h"

void AHomesteadWorld::BuildSeaCave()
{
    if (bSeaCaveBuilt) return;
    bSeaCaveBuilt = true;
    FActorSpawnParameters Spawn;
    Spawn.Owner = this;
    Spawn.ObjectFlags |= RF_Transient;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AHomesteadSeaCave* Cave = GetWorld()->SpawnActor<AHomesteadSeaCave>(AHomesteadSeaCave::StaticClass(), FTransform::Identity, Spawn);
    if (!Cave) return;
    if (!Cave->Build())
    {
        Cave->Destroy();
        return;
    }
    SeaCave = Cave;
}
