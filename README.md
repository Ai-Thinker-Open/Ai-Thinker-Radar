# Ai-Thinker-Radar SDK
This SDK is applicable to the Ai-Thinker Rd-01 module.

## clone
```
git clone --recurse-submodules https://github.com/Ai-Thinker-Open/Ai-Thinker-Radar.git
```
## limits of authority
```
cd Ai-Thinker-Radar/Ai-Thinker-WB2/toolchain/riscv/Linux
. chmod755.shs
cd ../../../..
```
## patches
```
cd Ai-Thinker-WB2
git apply ../patch/bfl_main.patch
cd ..
```
## build

```
cd Ai-Thinker-Radar/project
./genromap
```


