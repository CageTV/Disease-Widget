"""Builds the FOMOD installer zip: python tools/make_fomod.py <version> <new DiseaseWidget.dll> <older DiseaseWidget.dll>
Output: release/Disease Widget - SKSE - Movable <version>.zip  (kuroishitam's v1.1 + mouse dragging, see NOTICE.txt)."""
import os, sys, zipfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ver, new_dll, old_dll = sys.argv[1:4]
out_dir = os.path.join(HERE, "release")
os.makedirs(out_dir, exist_ok=True)
out = os.path.join(out_dir, f"Disease Widget - SKSE - Movable {ver}.zip")

INFO = f"""<fomod>
  <Name>Disease Widget - SKSE (movable)</Name>
  <Author>kuroishitam (mouse dragging and the 1.7.x build by CageTV)</Author>
  <Version>{ver}</Version>
  <Website>https://github.com/CageTV/Disease-Widget</Website>
  <Description>kuroishitam's Disease Widget - SKSE v1.1 with one addition: the widget can be moved with the mouse from its page in SKSE Menu Framework. A DiseaseWidget.dll for each game version: Skyrim 1.6.1170 and newer including 1.7.x, and VR / older.</Description>
</fomod>
"""
cfg = open(os.path.join(HERE, "tools", "_ModuleConfig.xml"), encoding="utf-8").read()
cfg = cfg.replace("Works on Skyrim SE/AE 1.6.x and on every later version, including 1.7.x. Pre-selected when the installer sees 1.6.1170 or newer.",
                  "Works on Skyrim SE/AE 1.6.1170 and on every later version, including 1.7.x. Pre-selected when the installer sees 1.6.1170 or newer. Includes the mouse dragging.")
cfg = cfg.replace("For Skyrim VR and Skyrim SE/AE 1.6.1130 and older. On 1.6.1170 either build works.",
                  "For Skyrim VR and Skyrim SE/AE 1.6.1130 and older. On 1.6.1170 either build works. Includes the mouse dragging.")
readme = open(os.path.join(HERE, "README.md"), encoding="utf-8").read()
notice = open(os.path.join(HERE, "NOTICE.txt"), encoding="utf-8").read()
data = os.path.join(HERE, "Data")
with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("fomod/info.xml", INFO)
    z.writestr("fomod/ModuleConfig.xml", cfg)
    z.writestr("README.txt", readme)
    z.writestr("NOTICE.txt", notice)
    z.write(new_dll, "new/SKSE/Plugins/DiseaseWidget.dll")
    z.write(old_dll, "older/SKSE/Plugins/DiseaseWidget.dll")
    for root, _, files in os.walk(data):
        for f in files:
            full = os.path.join(root, f)
            z.write(full, "common/" + os.path.relpath(full, data).replace("\\", "/"))
print("wrote", out, os.path.getsize(out))
