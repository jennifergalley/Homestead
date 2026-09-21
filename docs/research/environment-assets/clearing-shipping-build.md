# Clearing Shipping code, before staging

Run: `20260921-033354-2d257ba0`. These are genuine native build results, not
staging, gameplay, screenshot or environment acceptance.

## Real products

`Saved\Automation\20260921-033354-2d257ba0\clearing-shipping-build-02\final-native-product.json`
records the read-only PE/PDB and embedded-manifest inspection:

| Product | SHA-256 |
| --- | --- |
| Shipping executable | `A32A2A097203FD555776E66CE08B9487F1CA75DFDE9F332D49257BCCEF7897F1` |
| PDB | `4FFAD353E8843A246D9E88865A80A28141E77CA4C41BBD720F769F390D001AB6` |
| Embedded RT_MANIFEST 1, language 1033 | `BF5DCA0FE1C186D6C72539906FA23D85221ACB3F420DD096A4AE284849741BB6` |

Matching CodeView/PDB GUID: `cc230665-f3a7-0c7b-a21f-b9d86450d190`, age 2.
Executable size: 166,595,072 bytes. PDB size: 238,669,824 bytes.
The subsequent genuine UBT metadata-only step passed in `metadata\result.json`.
This original build does not yet enable the explicitly approved Shipping QA
actor gate; a later code build needs its own receipt and must not relabel these
bytes.

## Supported split, not a removed manifest

The original exact Shipping link could not create `mt.exe` under the root-only
job and failed 1158. Its failure is retained in
`clearing-shipping-build-01\link1`.

The separately approved split replaces `/MANIFEST:EMBED` with supported
`/MANIFEST` plus a fresh `/MANIFESTFILE`. The original engine manifest becomes
an unchanged input to the separate exact SDK `mt.exe` leaf. The only other
response modification is the already-verified resource-to-COFF substitution.
The linker exit-0 proof is `clearing-shipping-build-02\link1`.

The first separate manifest invocation incorrectly added optional
`-validate_manifest`, which requires a standalone definition identity absent
from the genuine input manifests. It failed 31/10100ba after modifying the owned
image. Both the partial image and failure remain in `manifest`. Only the
hash-verified owned pre-tool image was restored. The distinct normal
merge/embed invocation in `manifest-02` passed.

`Inspect-NativeModule.py` reads the actual PE resource without loading code.
It requires every generated/engine input element and attribute in the embedded
result, including UAC asInvoker/uiAccess=false, Common Controls 6 amd64,
supported Windows OS and long-path awareness. Its four negative/positive
manifest tests pass, and existing Editor DLL/PDB inspection still passes.

## Preserved qualifications

The first Shipping action export failed closed on a short-lived child whose
CIM path was null before handle acquisition. Root 48368, unknown child 25848
and identified conhost 13448 are recorded in `clearing-shipping-plan-01`.
Do not retrospectively identify the unknown child.

The corrected export uses the installed source-supported operation-local
`UnrealBuildTool_SourceFileWorkingSet__Provider=None`; it does not edit user
configuration or infer an identity for that failed observation.
`clearing-shipping-plan-02\actions.json` is the actual successful action export,
SHA-256 `8923CCD7EA23E7362401B865DAD8684EB877CF0F758B2DD363A037BCFD5D7F27`.

Guarded compile/link/conversion/manifest leaves admit no uploader/helper.
Their process/endpoint evidence is finite sampling, not continuous network or
filesystem history. The metadata-only managed UBT route has its separately
documented conhost allowance. No runtime or staged-candidate pass is inferred.
