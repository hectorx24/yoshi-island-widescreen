"""Create the isolated headless test AVD from verified local Android images."""
from pathlib import Path

root=Path(__file__).resolve().parents[1]
base=root/'local/android-toolchain'
image=base/'images/arm64-v8a'
if not (image/'system.img').exists(): raise SystemExit('System image extraction is incomplete')
avds=base/'avd'; avd=avds/'yoshi_test.avd'
avd.mkdir(parents=True,exist_ok=True)
(avds/'yoshi_test.ini').write_text(f'avd.ini.encoding=UTF-8\npath={avd}\ntarget=android-35\n')
values={'AvdId':'yoshi_test','avd.ini.encoding':'UTF-8','abi.type':'arm64-v8a',
        'hw.cpu.arch':'arm64','hw.cpu.ncore':'2','hw.ramSize':'2048',
        'vm.heapSize':'256','disk.dataPartition.size':'1024M','image.sysdir.1':str(image),
        'hw.lcd.width':'1280','hw.lcd.height':'720','hw.lcd.density':'240',
        'hw.gpu.enabled':'yes','hw.gpu.mode':'swiftshader_indirect','hw.keyboard':'yes',
        'showDeviceFrame':'no','hw.audioInput':'no','hw.audioOutput':'no',
        'hw.mainKeys':'no','hw.sdCard':'no','fastboot.forceColdBoot':'yes'}
(avd/'config.ini').write_text(''.join(f'{k}={v}\n' for k,v in values.items()))
print(f'ANDROID_AVD_HOME={avds}')
