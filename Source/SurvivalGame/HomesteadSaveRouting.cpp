#include "HomesteadSaveRouting.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

bool IsHomesteadPreviewProfile(const FString& Profile)
{
    if (Profile.Len() < 1 || Profile.Len() > 32 || Profile[0] < 'a' || Profile[0] > 'z') return false;
    for (const TCHAR C : Profile)
        if (!((C >= 'a' && C <= 'z') || (C >= '0' && C <= '9') || C == '-')) return false;
    return true;
}

bool ResolveHomesteadSaveRoute(const TCHAR* CommandLine, const FString& ProjectSavedRoot,
    const FString& UserSettingsRoot, const FString& TestOutput, FHomesteadSaveRoute& Route, FString& Error)
{
    Route = {};
    Error.Empty();
    const FString Name(TEXT("-HomesteadPreviewProfile"));
    const FString Prefix = Name + TEXT("=");
    const TCHAR* Cursor = CommandLine;
    FString Token, Profile;
    bool Present = false;
    while (FParse::Token(Cursor, Token, false))
    {
        if (!Token.StartsWith(Name, ESearchCase::IgnoreCase)) continue;
        if (Present || !Token.StartsWith(Prefix, ESearchCase::IgnoreCase))
        {
            Error = TEXT("Supply exactly one -HomesteadPreviewProfile=<id>; a bare, duplicate or malformed option is not allowed.");
            return false;
        }
        Present = true;
        Profile = Token.Mid(Prefix.Len());
        if (!IsHomesteadPreviewProfile(Profile))
        {
            Error = TEXT("Preview profile must be 1-32 ASCII lowercase letters/digits/hyphens, starting with a letter. No save files were accessed.");
            return false;
        }
    }
    if (FParse::Param(CommandLine, TEXT("HomesteadSmokeTest")) || FParse::Param(CommandLine, TEXT("HomesteadVisualPlaytest")))
    {
        Route.Mode = TEXT("test-sandbox");
        Route.Directory = FPaths::Combine(TestOutput, TEXT("SmokeSave"));
    }
    else if (Present)
    {
        if (UserSettingsRoot.IsEmpty() || FPaths::IsRelative(UserSettingsRoot))
        {
            Error = TEXT("The fixed user-settings root is unavailable. No save files were accessed.");
            return false;
        }
        Route.Mode = TEXT("preview");
        Route.Profile = Profile;
        // The fixed prefix also makes Windows reserved device identifiers harmless directory names.
        Route.Directory = FPaths::Combine(UserSettingsRoot, TEXT("SurvivalGame"), TEXT("PreviewProfiles"),
            TEXT("profile-") + Profile, TEXT("SaveGames"));
    }
    else
    {
        Route.Mode = TEXT("default");
        Route.Directory = FPaths::Combine(ProjectSavedRoot, TEXT("SaveGames"));
    }
    Route.Directory = FPaths::ConvertRelativePathToFull(Route.Directory);
    FPaths::NormalizeDirectoryName(Route.Directory);
    return true;
}
