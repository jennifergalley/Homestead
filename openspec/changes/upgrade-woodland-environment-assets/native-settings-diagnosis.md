# Bounded read-only diagnosis of the failed native settings probe

Subsequent17:57 decision: coordinator explicitly approved the settings-only
execution-disabled Python contract rather than the overbroad module-absence
proxy. Installed dependency modules are permitted, but interpreter,
startup-script/.pth/pip execution is not. One corrected build and distinct
third probe are conditional on direct entry/exit state measurements and all
existing containment/network/marker requirements. This does not pass the
old attempt or admit asset operations. The diagnosis below is historical.

2026-09-20,17:55 Arizona. Run20260920-182217-d1f84e39.
OpenSpec CLI reports4/29. Task1.3 remains unchecked. No implementation,
compilation, fixture, additional Editor launch or asset operation occurred
during this diagnosis. Both consumed reservations remain immutable.

## Config identifiers: exporter defect, isolation still unproved

Installed `Engine\Source\Runtime\Core\Private\Misc\ConfigCacheIni.cpp`
assigns known globals such as GEngineIni their logical names at6721-6724.
The name is not a physical destination. GetConfigFilename at5169 also
documents this distinction. The native exporter incorrectly applies a
physical-path containment assertion to those names.

Actual destination provenance is available from the existing config branch:
`FConfigBranch::IniPath`, declared in
`Engine\Source\Runtime\Core\Public\Misc\ConfigCacheIni.h:1136`.
Known-branch initialization saves the loaded destination into that member
at ConfigCacheIni.cpp:6434-6435. ConfigContext.cpp:489 obtains the destination
through GetDestIniFilename; ConfigCacheIni.cpp:6288 onward handles the
explicit INI command-line override. FindBranchWithNoReload at4258-4298
resolves existing branches without reloading files, although it updates the
branch's inactivity timer. Branch hierarchy, source directories and
command-line layers are also available for provenance.

Smallest correction: export the actual resolved branch and destination for
each redirected config, retain the logical identifier separately, and require
the actual destinations to equal the isolated files. Missing branches must
fail. Do not replace the current check with acceptance of arbitrary logical
names or with a recomputation of the requested path.

The real log records the intended explicit overrides. After shutdown the
isolated Config directory contains the other six requested files, but no
Engine.ini. SaveBranch at1329-1363 may delete a destination when no saved
output remains. This is a source-supported explanation for possible absence,
not proof that this particular deletion occurred. The existing evidence
cannot reconstruct the actual Engine/GameUserSettings branch destinations.

## Python: dependency re-enablement, not a comma-parser failure

PluginManager.cpp:1469-1485 explicitly parses comma-separated plugin names.
The disable pass at1587-1601 configures those roots; it is not a permanent
dependency blacklist. Dependency expansion at2549-2597 queues dependencies
without consulting the root configured-name set. Platform/configuration/
target filtering is performed separately at2424-2437.

Three actually enabled descriptors directly require PythonScriptPlugin:
Niagara.uplugin:19-20, SequencerScripting.uplugin:19-20 and
RigVM.uplugin:46-50. PluginUtils requires EditorScriptingUtilities at25-27;
its own optional Python reference is explicitly false, so it is not counted
as a direct Python-enabling edge.

A data-only reverse-dependency calculation over the242 actually enabled
descriptors yields a conservative56-plugin closure for excluding both
PythonScriptPlugin and EditorScriptingUtilities. It includes Interchange,
InterchangeEditor, Niagara and RigVM. The exact graph, descriptor hashes and
causal edges are retained in the diagnosis evidence. This calculation filters
enabled references and Editor/Win64 allow/deny lists; it is not a full replay
of UE receipt, optional-dependency, version or configuration semantics, nor
a runtime-validated replacement command line.

Consequently, removing only the three immediate Python dependents is not
a justified complete correction. Keeping the existing no-Python-module
contract requires a reviewed dependency-closed plugin selection, with import
capability consequences explicitly addressed. No speculative disable list
was applied. Treating an inert loaded module as acceptable would instead
require an explicit policy decision; this diagnosis does not make one.

The actual log at1569 says Python is disabled by -DisablePython.
PythonScriptPlugin.cpp:115-118 returns false for that flag.
ConfigureAndInitializePython at988-1010 sets availability and returns before
InitializePython when unavailable. Pip setup/install and commandlet startup
callbacks are within the initialization path at1338-1389; RunStartupScripts
also checks interpreter initialization at1611-1616. The preload module
separately calls LoadPythonLibraries at22-25 and loads native libraries at67,
without that DisablePython check. Plugin/DLL loading is therefore distinct
from interpreter initialization and script/pip execution.

No startup-script or pip execution was evidenced. No pip directory exists.
The standard disabled initialization path is source-supported, but the
existing exporter did not record public configured/available/initialized
states or CPython interpreter state. In particular, public IsPythonInitialized
at717-724 returns fully-initialized state, not the private interpreter-created
flag. Do not retrospectively claim direct interpreter-state proof.

## Denied AutoSDK/UBT request: optional startup validation

TargetPlatformManagerModule.cpp:366-410 starts ValidatePlatforms/
OutputSDKs/AllPlatforms through InvokeUnrealBuildToolAsync during early
FileSystemReady initialization. This is the caller matching the denied
Build.bat/UBT request and warning in the real log.

The supported child-local suppression is UE_SKIP_UBT_SDK_SETUP=1:
the exact variable is declared at54 and tested at221-227 before other
AutoSDK logic. Clearing UE_SDKS_ROOT alone is insufficient; the Editor
normally still validates platform status without AutoSDK at229-239.
The warning's wording therefore does not prove UE_SDKS_ROOT was present.

Smallest correction for a future settings-only probe is that child-local
environment value, not a guessed telemetry flag or global change. It omits
SDK validation, so it cannot prove or replace subsequent cook/build SDK
readiness. The denied request did not execute a helper; actual final job
accounting remains one total process.

## DDC: existing assertion rejects a structural adapter

Applying the unchanged Assert-AuthoringDdc offline to the actual JSON throws
"An effective DDC store is not admitted local file/memory storage."
The first rejected node is Async, not the intended File System destination.

DerivedDataCacheStoreAsync.cpp:134-139 creates an unnamed local Async stats
node and places the inner cache beneath it. This is the asynchronous adapter.
DerivedDataCacheStoreHierarchy.cpp:1666-1675 appends real store-stat children
to its container, explaining the intervening unnamed node. The real record
then contains exactly one local File System node at the isolated DDC path.

Smallest correction: narrowly classify these source-confirmed structural
nodes separately from storage, retain complete child traversal/provenance,
and still require exactly one local filesystem store at the exact fresh
path. Reject unknown types, remote stores, path changes and malformed
structural nodes. No policy assertion was changed or retroactively passed.

## Disposition and evidence

Generated source hashes, unchanged-policy offline rejection and descriptor
graph are under
`docs\research\environment-assets\native-settings-diagnosis-01`.
Original runtime proof remains indexed by
`docs\research\environment-assets\native-settings-02\receipt.json`.

The exporter, AutoSDK setting and DDC classification have narrow supported
corrections. Python dependency selection remains a material workflow decision,
not a one-flag fix. Actual config provenance and successful cooperative stop
also remain unproved. No further launch, build, import, cook, Pak or Shipping
operation is authorized. The selected human release and saves are unchanged;
there is no environment visual upgrade. Return to coordinator review.
