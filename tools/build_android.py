"""Incremental private ARM64 APK build using the existing SDK/SDL source cache."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

def main():
    root=Path(__file__).resolve().parents[1]
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',required=True,type=Path)
    args=p.parse_args()
    raw=args.rom.read_bytes()
    if len(raw)%32768==512: raw=raw[512:]
    if hashlib.sha256(raw).hexdigest()!='bd763c1a56365c244be92e6cffefd318780a2a19eda7d5baf1c6d5bd6c1b3e06':
        raise SystemExit('USA v1.1 identity mismatch')
    java=Path(os.environ.get('JAVA_HOME','/Users/hectorx24/Downloads/patreon/tooling/jdk-17.0.20.1+1/Contents/Home'))
    sdk=Path(os.environ.get('ANDROID_HOME','/Users/hectorx24/Downloads/patreon/tooling/sdk'))
    ndk=Path(os.environ.get('ANDROID_NDK_HOME',str(root/'local/android-toolchain/ndk')))
    bt=sdk/'build-tools/35.0.0'; jar=sdk/'platforms/android-35/android.jar'
    env=dict(os.environ,JAVA_HOME=str(java),PATH=str(java/'bin')+os.pathsep+os.environ['PATH'])
    def run(*cmd): subprocess.run([str(c) for c in cmd],cwd=root,env=env,check=True)
    cmake=root/'local/venv/bin/cmake'
    env['PATH']=str(cmake.parent)+os.pathsep+env['PATH']
    build=root/'build/android-arm64'; out=root/'local/android'
    for d in ['assets','classes','dex','artifacts']: (out/d).mkdir(parents=True,exist_ok=True)
    sdl=root/'build/_deps/sdl3-src'
    run(cmake,'-S',root,'-B',build,'-G','Ninja',f'-DCMAKE_TOOLCHAIN_FILE={ndk}/build/cmake/android.toolchain.cmake',
        '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-26','-DANDROID_STL=c++_static',
        '-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON','-DCMAKE_BUILD_TYPE=Release')
    run(cmake,'--build',build,'--parallel','4')
    sources=list((sdl/'android-project/app/src/main/java').rglob('*.java'))+list((root/'android/java').rglob('*.java'))
    run(java/'bin/javac','-source','8','-target','8','-bootclasspath',jar,'-d',out/'classes',*sources)
    classes=list((out/'classes').rglob('*.class'))
    run(bt/'d8','--lib',jar,'--min-api','26','--output',out/'dex',*classes)
    (out/'assets/yoshi-usa11.sfc').write_bytes(raw)
    shutil.copy2(root/'tests/boot.script',out/'assets/boot.script')
    shutil.copy2(root/'tests/tutorial.script',out/'assets/tutorial.script')
    unsigned=out/'artifacts/unsigned.apk'
    run(bt/'aapt','package','-f','--debug-mode','-M',root/'android/AndroidManifest.xml','-I',jar,'-A',out/'assets','-F',unsigned)
    with zipfile.ZipFile(unsigned,'a',compression=zipfile.ZIP_DEFLATED) as z:
        z.write(out/'dex/classes.dex','classes.dex')
        z.write(build/'libYoshiIsland.so','lib/arm64-v8a/libYoshiIsland.so')
        z.write(build/'sdl3/libSDL3.so','lib/arm64-v8a/libSDL3.so')
    key=out/'private-dev.keystore'
    if not key.exists():
        run(java/'bin/keytool','-genkeypair','-keystore',key,'-storepass','android','-keypass','android',
            '-alias','androiddebugkey','-dname','CN=Android Debug','-keyalg','RSA','-keysize','2048','-validity','10000')
    aligned=out/'artifacts/aligned.apk'; apk=out/'artifacts/yoshi-usa11-arm64-dev.apk'
    run(bt/'zipalign','-P','16','-f','4',unsigned,aligned)
    run(bt/'apksigner','sign','--ks',key,'--ks-pass','pass:android','--out',apk,aligned)
    run(bt/'apksigner','verify','--verbose',apk)
    run(bt/'aapt','dump','badging',apk)
    run(sys.executable,root/'tools/checkpoint.py','--event','Private ARM64 development APK built and package/signature verified; device execution unverified')
    print(apk)

if __name__=='__main__': main()
