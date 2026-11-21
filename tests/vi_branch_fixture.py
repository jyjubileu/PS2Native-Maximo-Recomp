"""Generate synthetic VIF1 inputs and a standalone EE ELF for the VI timing tests.

The ELF uploads the same stream through DMA1 and prints the result qwords via
SIO. All instructions and data below were authored for the regression tests.
"""
import argparse
import struct
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("output", type=Path)
variants = parser.add_mutually_exclusive_group()
variants.add_argument("--without-marker", action="store_true")
variants.add_argument("--regressions", action="store_true")
variants.add_argument("--continuations", action="store_true")
opts = parser.parse_args()
out = opts.output
out.mkdir(parents=True, exist_ok=True)

pack=lambda *x: struct.pack(f"<{len(x)}I", *x)
NOP=0x8000033C
U_NOP=0x000002FF
E=0x40000000
def iadd(rt,k): return 0x10000000|rt<<16|k
def ilw(rt,a): return 0x08400000|rt<<16|a
def isw(rt,a): return 0x0B000000|rt<<16|a
def br(major,left,right,at,target): return major<<25|left<<11|right<<16|((target-at-1)&0x7ff)
def special_it(op,rt): return 0x8000003c|rt<<16|(op//4)<<6|(op%4)

def regression_stream():
    micro=[(NOP,U_NOP)]*216
    scenarios=[
        ("two-stall-cycles",0,False,False,False),
        ("one-stall-cycle",1,False,False,False),
        ("no-stall",2,False,False,False),
        ("ilwr",0,True,False,False),
        ("six-writes",0,False,True,False),
        ("stalled-write-chain",0,False,False,True),
        ("ibeq-unequal",0,False,False,False),
        ("ibne-unequal",0,False,False,False),
    ]
    def pair(word,upper=U_NOP): return word,upper
    for index,(name,gap,indirect,repeated,stall_between) in enumerate(scenarios,1):
        addr=0x60+index
        pre=[pair(iadd(2,0)),pair(iadd(3,9)),pair(iadd(4,7)),pair(iadd(6,0x20)),
             pair(iadd(1,1)),pair(NOP),pair(NOP),pair(NOP)]
        if stall_between:
            # Back-up VI2=0, stall on a full vector lane, then restart the backup at VI2=1.
            add_vf8=0x01000228
            add_from_vf8=0x01004268
            pre.extend([pair(iadd(2,1),add_vf8),pair(iadd(2,2),add_from_vf8)])
        elif repeated:
            pre.extend(pair(iadd(2,v)) for v in (3,4,5,6,7,1))
        elif name.endswith("unequal"):
            pre.extend([pair(iadd(2,2)),pair(NOP)])
        else:
            load = special_it(0x3E,1)|(2<<21)|(6<<11) if indirect else ilw(1,0x20)
            pre.extend([pair(load),*[pair(NOP)]*gap,pair(iadd(2,1))])
        cmp=0x29 if name=="ibne-unequal" else 0x28
        at=len(pre)
        pre.extend([pair(br(cmp,2,1,at,at+5)),pair(iadd(8,0x53)),
                    pair(isw(3,addr)),pair(br(0x20,0,0,at+3,at+6)),pair(NOP),
                    pair(isw(4,addr)),pair(0x0A880000|addr,U_NOP|E),pair(NOP)])
        start=index*24
        assert len(pre)<=24
        micro[start:start+len(pre)]=pre
    stream=[0x01000404,0x03000040,0x02000010,0x04000003,0x05000000,0x20000000,0,
            0x6C010020,0,0,1,0,0x4A000000|(len(micro)<<16)]
    for lo,hi in micro:
        stream.extend((lo,hi))
    for i in range(1,9):
        stream.extend((0x14000000|24*i,0x11000000))
    while len(stream)%4: stream.append(0)
    return pack(*stream),micro,list(range(0x61,0x69))

def continuation_stream():
    micro=[(NOP,U_NOP)]*224
    for i,need_load in enumerate((True,False),1):
        addr=0x70+i
        start=i*24
        target=0x90+i*16
        pre=[iadd(2,0),iadd(3,9),iadd(4,7),iadd(10,target),iadd(1,0 if need_load else 1),NOP,NOP,NOP]
        if need_load: pre += [ilw(1,0x20)]
        pre.extend([0x48000000 | (10<<11),iadd(2,1)])
        micro[start:start+len(pre)]=[(x,U_NOP) for x in pre]
        tail=[br(0x28,2,1,target,target+5),iadd(8,0x53),isw(3,addr),
              br(0x20,0,0,target+3,target+6),NOP,isw(4,addr),0x0A880000|addr,NOP]
        micro[target:target+8]=[(x,U_NOP|(E if n==6 else 0)) for n,x in enumerate(tail)]
    vif=[0x01000404,0x03000040,0x02000010,0x04000003,0x05000000,0x20000000,0,
         0x6C010020,0,0,1,0,0x4A000000|(len(micro)<<16)]
    for lo,hi in micro:
        vif.extend((lo,hi))
    for i in (1,2):
        vif.extend((0x14000000|24*i,0x11000000))
    while len(vif)%4: vif.append(0)
    return pack(*vif),micro,[0x71,0x72]

if opts.continuations:
    stream,micro,addresses=continuation_stream()
    (out/"minimo-vif1.bin").write_bytes(stream)
    (out/"minimo-vu1.bin").write_bytes(b"".join(pack(*x) for x in micro))
elif opts.regressions:
    stream,micro,addresses=regression_stream()
    (out/"minimo-vif1.bin").write_bytes(stream)
    (out/"minimo-vu1.bin").write_bytes(b"".join(pack(*x) for x in micro))
else:
    micro=[(NOP,U_NOP)]*192
    def test(start,case,addr):
        pre=[iadd(2,0),iadd(3,9),iadd(4,7),iadd(7,1),iadd(1,1),NOP,NOP,NOP]
        if case in ("load+beq","load+bne","load_other"):
            pre.append(ilw(1,0x20))
        else:
            pre.append(NOP)
        pre.append(iadd(2,1))
        if case=="separated":
            pre.append(NOP)
        cmp=7 if case=="load_other" else 1
        major=0x29 if case=="load+bne" else 0x28
        at=len(pre)
        pre += [br(major,2,cmp,at,at+5),NOP,isw(3,addr),br(0x20,0,0,at+3,at+6),NOP,isw(4,addr),NOP,NOP]
        buf=[(x,U_NOP) for x in pre]
        buf[-2]=(NOP,U_NOP|E)
        micro[start:start+len(buf)]=buf

    for i,case in enumerate(("load+beq","no_stall","load+bne","separated","load_other"),1):
        test(i*24,case,0x30+i)
    # Sixth program checks VIF top and itop values observed by VU1.
    micro[144:151]=[
        (special_it(0x68,5),U_NOP),(special_it(0x69,6),U_NOP),
        (NOP,U_NOP),(isw(5,0x36),U_NOP),(isw(6,0x37),U_NOP),(NOP,U_NOP|E),(NOP,U_NOP)
    ]
    # Make the existing runtime's issue-cycle scheduler active in its control build.
    # Kept in an unexecuted location so the fixture measures VI and not a flag result.
    if not opts.without_marker:
        micro[191]=(0x38010000,U_NOP)
    vif=[0x01000404,0x03000040,0x02000010,0x04000003,0x05000000,0x20000000,0,
         0x6C010020,0,0,1,0,0x4A000000|(len(micro)<<16)]
    for lower,upper in micro:
        vif.extend((lower,upper))
    for i in range(1,7):
        vif.extend((0x14000000|24*i,0x11000000))
    # Six starts return TOPS to the initial bank and TOP to the alternate bank.
    # Exercise TOPS-relative memory with a value independent of the VI cases.
    vif.extend((0x6C018005,0x11223344,0x89abcdef,0x08050607,0xAABBCCDD))
    while len(vif)%4: vif.append(0)
    stream=pack(*vif)
    (out/"minimo-vif1.bin").write_bytes(stream)
    (out/"minimo-vu1.bin").write_bytes(b"".join(pack(*x) for x in micro))

    addresses=(0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x45)

# Minimal R5900 ELF delivering the exact stream to DMA1 and returning a raw memory sample over EE SIO.
base=0x100000
image=bytearray(0x5000)
image[0x1000:0x1000+len(stream)]=stream
image[0x2000:0x2010]=b"0123456789ABCDEF"
words=[]
def emit(*x): words.extend(x)
def imm(op,rt,rs,k): return op<<26|rs<<21|rt<<16|(k&0xffff)
def li(reg,value): emit(imm(0x0f,reg,0,value>>16),imm(0x0d,reg,reg,value))
def sw(reg,where,o): emit(imm(0x2b,reg,where,o))
def jump(op,rs,rt,where): emit(imm(op,rt,rs,where-len(words)-1),0)
def form(rs,rt,rd,s,fn): return rs<<21|rt<<16|rd<<11|s<<6|fn
def sb(r,where): emit(imm(0x28,r,where,0))
li(8,0x1000E000); li(9,1); sw(9,8,0)
li(8,0x10003c00); sw(9,8,0x10)
li(8,0x10009000); li(9,base+0x1000); sw(9,8,0x10)
li(9,len(stream)//16); sw(9,8,0x20)
li(9,0x101); sw(9,8,0)
n=len(words)
emit(imm(0x23,9,8,0),imm(0x0c,9,9,0x100))
jump(5,9,0,n)
li(8,0x10003c00)
n=len(words)
emit(imm(0x23,9,8,0),imm(0x0c,9,9,0x0f))
jump(5,9,0,n)
li(8,0x1000F180); li(10,base+0x2000)
for address in addresses:
    for letter in f"REX3 {address:03X} ".encode():
        li(13,letter); sb(13,8)
    li(9,0x1100C000+address*16)
    li(15,16)
    n=len(words)
    emit(imm(0x24,11,9,0),form(0,11,12,4,2),form(12,10,12,0,0x21),imm(0x24,13,12,0)); sb(13,8)
    emit(imm(0x0c,12,11,15),form(12,10,12,0,0x21),imm(0x24,13,12,0)); sb(13,8)
    emit(imm(9,9,9,1),imm(9,15,15,-1))
    jump(5,15,0,n)
    li(13,10); sb(13,8)
for letter in b"REX3 FIN\n":
    li(13,letter); sb(13,8)
jump(4,0,0,len(words))
assert len(words)*4<=0x1000
image[:len(words)*4]=pack(*words)
header=b"\x7fELF\x01\x01\x01"+bytes(9)+struct.pack("<HHIIIIIHHHHHH",2,8,1,base,52,0,0x20924001,52,32,1,0,0,0)
ph=struct.pack("<8I",1,0x1000,base,base,len(image),len(image),7,0x1000)
elf=out/"minimo.elf"
elf.write_bytes((header+ph).ljust(0x1000,b"\0")+image)
