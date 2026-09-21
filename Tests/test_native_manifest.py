import importlib.util
import pathlib
import unittest


spec = importlib.util.spec_from_file_location(
    "native_inspector", pathlib.Path(__file__).resolve().parents[1] / "Scripts" / "Inspect-NativeModule.py")
inspector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inspector)


class ManifestTests(unittest.TestCase):
    generated = b'<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0"><trustInfo xmlns="urn:schemas-microsoft-com:asm.v3"><security><requestedPrivileges><requestedExecutionLevel level="asInvoker" uiAccess="false"/></requestedPrivileges></security></trustInfo></assembly>'
    engine = b'<assembly xmlns="urn:schemas-microsoft-com:asm.v1"><dependency><dependentAssembly><assemblyIdentity name="Microsoft.Windows.Common-Controls" version="6.0.0.0" processorArchitecture="amd64"/></dependentAssembly></dependency></assembly>'
    merged = generated.replace(b'</assembly>', engine.split(b'>', 1)[1])

    def test_preserves_both_inputs(self):
        inspector.require_manifest_inputs(self.merged, [self.generated, self.engine])

    def test_namespace_aliases_are_not_content_changes(self):
        inspector.require_manifest_inputs(self.merged, [
            self.generated.replace(b"asm.v3", b"asm.v2"), self.engine])

    def test_rejects_missing_or_changed_contract(self):
        for bad in (self.generated, self.engine, self.merged.replace(b'asInvoker', b'requireAdministrator'),
                    self.merged.replace(b'amd64', b'x86'), self.merged.replace(b'6.0.0.0', b'5.0.0.0')):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                inspector.require_manifest_inputs(bad, [self.generated, self.engine])

    def test_rejects_unsafe_or_empty_inputs(self):
        for bad in (b'', b'<!DOCTYPE assembly><assembly/>', b'<other/>'):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                inspector.require_manifest_inputs(bad, [self.generated])
        with self.assertRaises(ValueError):
            inspector.require_manifest_inputs(self.merged, [])


if __name__ == "__main__":
    unittest.main()
