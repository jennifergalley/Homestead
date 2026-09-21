"""Identify durable receipt inputs without depending on Blender."""
import re


def is_receipt_file(path):
    return (path.is_file() and path.suffix.lower() != ".bak"
            and re.search(r"\.blend\d+$", path.name, re.IGNORECASE) is None)
