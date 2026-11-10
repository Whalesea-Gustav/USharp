"""Bind the queue reconstruction to retained BMW bytes and canonical REA records."""
import hashlib
import json
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[6]
sys.path.insert(0, str(root / "Tools/WukongAnimationPipeline"))
from inspect_calliope_native import EXE, EXPECTED_SHA, Image

original = root / "Saved/Research/ErlangHuman/P3-OuterCommit-20261016/NativeEcsDeferred/Results1"
output = Path(__file__).resolve().parents[1] / "ResearchArtifacts/GameThreadDefer"
output.mkdir(parents=True, exist_ok=True)
with EXE.open("rb") as stream:
    assert hashlib.file_digest(stream, "sha256").hexdigest() == EXPECTED_SHA
image = Image(EXE)
records = []
for name, rva in (("DeferredEnqueue", 0xae63230), ("DeferredFlush", 0xae5c970)):
    retained = json.loads((original / (name + ".json")).read_text())
    evidence = retained.get("structuredContent") or json.loads(retained["content"][0]["text"])
    assert evidence["subject"]["digest"]["sha256"] == EXPECTED_SHA
    assert evidence["parameters"]["procedure"] == hex(image.base + rva)
    body = image.capture_chained(rva)
    (output / (name + "-instructions.json")).write_text(json.dumps(body, indent=2))
    rows = {row["rva"]: row for chunk in body.get("chunks", [body]) for row in chunk["instructions"]}
    expected = {
        "DeferredEnqueue": {
            "0xae63292": ("shr", "ax, 3"),
            "0xae63296": ("add", "ax, 2"),
            "0xae632b7": ("lea", "eax, [rdx + 0x1000000]"),
            "0xae632d0": ("movsxd", "r8, dword ptr [rdi + 0x30]"),
            "0xae6331a": ("lea", "rcx, [rax + rcx*8]"),
            "0xae63326": ("add", "dword ptr [rdi + 0x30], r12d"),
        },
        "DeferredFlush": {
            "0xae5c9d5": ("call", "qword ptr [rax + 0x250]"),
            "0xae5ca2f": ("shr", "rax, 0x24"),
            "0xae5ca46": ("call", "qword ptr [rax + 0xf0]"),
            "0xae5ca86": ("cmp", "r14d, dword ptr [rsi + 0x30]"),
            "0xae5ca90": ("mov", "dword ptr [rsi + 0x30], r12d"),
        },
    }[name]
    for address, instruction in expected.items():
        assert (rows[address]["mnemonic"], rows[address]["operands"]) == instruction
    records.append({"name": name, "evidence_id": evidence["evidence_id"], "source": str(original / (name + ".json"))})
(output / "evidence-index.json").write_text(json.dumps({"sha256": EXPECTED_SHA, "records": records}, indent=2))
print("PASS: BMW digest, REA subjects/procedures and queue arithmetic/dispatch/destructor instruction checks")
