"""Headless libretro oracle for deterministic wait/press/dump/quit replays.

Use only a locally compiled reference core. ROM and captures stay local.
"""
import argparse
import ctypes as C
import hashlib
import json
import math
from pathlib import Path

class GameInfo(C.Structure):
    _fields_=[('path',C.c_char_p),('data',C.c_void_p),('size',C.c_size_t),('meta',C.c_char_p)]

class Variable(C.Structure):
    _fields_=[('key',C.c_char_p),('value',C.c_char_p)]

def run(core_path, rom, script, output):
    core=C.CDLL(str(core_path.resolve()))
    output.mkdir(parents=True,exist_ok=True)
    state={'format':0,'frame':0,'input':0,'image':None,'samples':0,'energy':0}
    variables={}
    env_type=C.CFUNCTYPE(C.c_bool,C.c_uint,C.c_void_p)
    video_type=C.CFUNCTYPE(None,C.c_void_p,C.c_uint,C.c_uint,C.c_size_t)
    audio_type=C.CFUNCTYPE(None,C.c_int16,C.c_int16)
    batch_type=C.CFUNCTYPE(C.c_size_t,C.POINTER(C.c_int16),C.c_size_t)
    poll_type=C.CFUNCTYPE(None)
    input_type=C.CFUNCTYPE(C.c_int16,C.c_uint,C.c_uint,C.c_uint,C.c_uint)
    @env_type
    def env(cmd,data):
        if cmd==16: # legacy core-option registration, first value is default
            values=C.cast(data,C.POINTER(Variable))
            i=0
            while values[i].key:
                variables[values[i].key]=values[i].value.split(b';',1)[1].strip().split(b'|')[0]
                i+=1
            return True
        if cmd==15:
            var=C.cast(data,C.POINTER(Variable))
            var[0].value=variables.get(var[0].key)
            return var[0].value is not None
        if cmd==10:
            state['format']=C.cast(data,C.POINTER(C.c_int))[0]
            return state['format'] in (0,1,2)
        if cmd==3: # may duplicate previous video frame
            C.cast(data,C.POINTER(C.c_bool))[0]=True
            return True
        if cmd==18: # support-no-game flag
            return True
        return False
    @video_type
    def video(data,w,h,pitch):
        if data:
            state['image']=(C.string_at(data,pitch*h),w,h,pitch,state['format'])
    @audio_type
    def audio(l,r):
        state['samples']+=2
        state['energy']+=int(l)*int(l)+int(r)*int(r)
    @batch_type
    def batch(data,frames):
        state['samples']+=2*frames
        state['energy']+=sum(int(data[i])**2 for i in range(2*frames))
        return frames
    @poll_type
    def poll(): pass
    @input_type
    def inp(port,device,index,button):
        if port or device!=1: return 0
        return 1 if state['input'] & (1<<button) else 0
    callbacks=[('environment',env),('video_refresh',video),('audio_sample',audio),
               ('audio_sample_batch',batch),('input_poll',poll),('input_state',inp)]
    for name,cb in callbacks:
        getattr(core,'retro_set_'+name)(cb)
    core.retro_init()
    core.retro_load_game.argtypes=[C.POINTER(GameInfo)]
    core.retro_load_game.restype=C.c_bool
    raw=rom.read_bytes()
    if len(raw)%32768==512: raw=raw[512:]
    buf=C.create_string_buffer(raw)
    game=GameInfo(str(rom.resolve()).encode(),C.cast(buf,C.c_void_p),len(raw),None)
    if not core.retro_load_game(C.byref(game)): raise RuntimeError('Oracle rejected ROM')
    core.retro_get_memory_data.argtypes=[C.c_uint]
    core.retro_get_memory_data.restype=C.c_void_p
    core.retro_get_memory_size.argtypes=[C.c_uint]
    core.retro_get_memory_size.restype=C.c_size_t
    def frames(n,mask=0):
        state['input']=mask
        for _ in range(n):
            core.retro_run()
            state['frame']+=1
    def dump(tag):
        for name,ident in [('wram',2),('sram',0),('vram',3)]:
            ptr,size=core.retro_get_memory_data(ident),core.retro_get_memory_size(ident)
            if ptr and size: (output/(tag+'.'+name+'.bin')).write_bytes(C.string_at(ptr,size))
        if state['image']:
            b,w,h,pitch,fmt=state['image']; pixels=bytearray()
            for y in range(h):
                for x in range(w):
                    offset=y*pitch+x*(4 if fmt==1 else 2)
                    v=int.from_bytes(b[offset:offset+(4 if fmt==1 else 2)],'little')
                    if fmt==1: rgb=((v>>16)&255,(v>>8)&255,v&255)
                    elif fmt==2:
                        # Snes9x BUILD_PIXEL_RGB565 stores original SNES green
                        # in bits 6..10, with one redundant bit at bit 5.
                        c=((v>>11)&31,(v>>6)&31,v&31)
                        rgb=tuple((x<<3)|(x>>2) for x in c)
                    else: rgb=(((v>>10)&31)*255//31,((v>>5)&31)*255//31,(v&31)*255//31)
                    pixels.extend(rgb)
            (output/(tag+'.ppm')).write_bytes(('P6\n%d %d\n255\n'%(w,h)).encode()+pixels)
        print(json.dumps({'tag':tag,'frame':state['frame'],'audio_samples':state['samples'],
                          'audio_rms':math.sqrt(state['energy']/max(1,state['samples']))}),flush=True)
    keys={k:i for i,k in enumerate(['b','y','select','start','up','down','left','right','a','x','l','r'])}
    for line in script.read_text().splitlines():
        parts=line.split('#')[0].split()
        if not parts: continue
        cmd=parts[0]
        if cmd=='turbo': continue
        if cmd=='wait': frames(int(parts[1]))
        elif cmd=='press':
            mask=sum(1<<keys[k] for k in parts[1].split('+'))
            frames(int(parts[2]) if len(parts)>2 else 1,mask)
            frames(1)
        elif cmd=='dump': dump(parts[1])
        elif cmd=='quit': break
        else: raise ValueError('Unsupported oracle command: '+cmd)
    core.retro_unload_game()
    core.retro_deinit()

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ['core','rom','script','output']: p.add_argument('--'+key,required=True,type=Path)
    args=p.parse_args()
    run(args.core,args.rom,args.script,args.output)
