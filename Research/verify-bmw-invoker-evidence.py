"""Recheck source bytes supporting the recovered invoker contract, without executing BMW."""
import hashlib
import json
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[6]
sys.path.insert(0, str(root / "Tools/WukongAnimationPipeline"))
from inspect_calliope_native import Image, EXE, EXPECTED_SHA

artifacts = Path(__file__).resolve().parents[1] / "ResearchArtifacts/BmwInvoker"
capture = json.loads((artifacts / "instructions.json").read_text())
evidence = json.loads((artifacts / "rea-set-invoker.json").read_text(encoding="utf-8-sig"))
assert evidence["subject"]["digest"]["sha256"] == capture["exe_sha256"] == EXPECTED_SHA
with EXE.open("rb") as stream:
    assert hashlib.file_digest(stream, "sha256").hexdigest() == EXPECTED_SHA
image = Image(EXE)
rows = {row["rva"]: row for chunk in capture["target"].get("chunks", [capture["target"]])
        for row in chunk["instructions"]}
for row in rows.values():
    assert image.pe.get_data(int(row["rva"], 16), len(bytes.fromhex(row["bytes"]))) == bytes.fromhex(row["bytes"])
for address, instruction in {
    "0xae5b39f": ("mov", "rsi, r8"),
    "0xae5b4bb": ("cmp", "dword ptr [rax + 0x10], 0"),
    "0xae5b4bf": ("jl", "0x14ae5b4e7"),
    "0xae5b4c1": ("mov", "qword ptr [rax + 8], rsi"),
    "0xae5b4c8": ("mov", "qword ptr [rcx + 0xd8], rsi"),
    "0xae5b4dd": ("mov", "qword ptr [rcx + rdx*8 + 8], rsi"),
    "0xae5b4e2": ("mov", "rax, qword ptr [rax]"),
}.items():
    assert (rows[address]["mnemonic"], rows[address]["operands"]) == instruction
print("PASS: executable digest, retained instruction bytes, direct invoker writes and return")
