#include "yoshi_scene_map.h"
#include <assert.h>
#include <string.h>
int main(void) {
    uint8_t out[1100]; size_t written;
    const uint8_t mixed[] = {2,'a','b','c',0x23,'x',0x44,'y','z',0x62,254,0x83,0,0,255};
    const uint8_t expected[] = {'a','b','c','x','x','x','x','y','z','y','z','y',254,255,0,'a','b','c','x'};
    assert(yoshi_asset_decode(mixed,sizeof(mixed),out,sizeof(out),&written));
    assert(written==sizeof(expected) && !memcmp(out,expected,written));
    const uint8_t overlap[] = {0,'a',0x87,0,0,255};
    assert(yoshi_asset_decode(overlap,sizeof(overlap),out,sizeof(out),&written));
    assert(written==9); for (size_t i=0;i<written;++i) assert(out[i]=='a');
    const uint8_t extended[] = {0xe4,255,'q',255};
    assert(yoshi_asset_decode(extended,sizeof(extended),out,sizeof(out),&written) && written==256);
    const uint8_t badcopy[] = {0x80,0,0,255};
    assert(!yoshi_asset_decode(badcopy,sizeof(badcopy),out,sizeof(out),&written));
    assert(!yoshi_asset_decode(mixed,sizeof(mixed)-1,out,sizeof(out),&written));
    assert(!yoshi_asset_decode(mixed,sizeof(mixed),out,1,&written));
    const uint8_t truncated[] = {0x44,'x'};
    assert(!yoshi_asset_decode(truncated,sizeof(truncated),out,sizeof(out),&written));
    assert(!yoshi_bg2_asset(NULL,0,0,out));
}
