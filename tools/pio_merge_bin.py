# Script pós-compilação do PlatformIO: gera release/historias_sinistras.bin (imagem
# completa para gravar em 0x0) e release/historias_sinistras_app.bin (somente o app, 0x10000).
import os
import shutil

Import("env")  # noqa: F821  (fornecido pelo PlatformIO)


def merge_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    out_dir = os.path.join(project_dir, "release")
    os.makedirs(out_dir, exist_ok=True)

    app = os.path.join(build_dir, env.subst("${PROGNAME}.bin"))
    parts = [(int(off, 16) if isinstance(off, str) else off, env.subst(img))
             for off, img in env.get("FLASH_EXTRA_IMAGES", [])]
    parts.append((int(env.subst("$ESP32_APP_OFFSET"), 16), app))
    parts.sort()

    args = []
    for off, img in parts:
        args += ["0x%x" % off, '"%s"' % img]
    merged = os.path.join(out_dir, "historias_sinistras.bin")
    cmd = " ".join([
        '"$PYTHONEXE"', '"%s"' % os.path.join(env.PioPlatform().get_package_dir("tool-esptoolpy") or "", "esptool.py"),
        "--chip esp32s3 merge-bin -o", '"%s"' % merged,
        "--flash-mode dio --flash-freq 80m --flash-size 8MB", " ".join(args)])
    if env.Execute(cmd) != 0:
        print("Aviso: não foi possível gerar a imagem completa; use os arquivos separados.")
    shutil.copyfile(app, os.path.join(out_dir, "historias_sinistras_app.bin"))
    print("Imagem completa (gravar em 0x0):  %s" % merged)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)  # noqa: F821
