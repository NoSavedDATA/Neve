# Neve Compiler

Documentation: [https://neve-lang.dev](https://neve-lang.dev)

Official repository of: [https://arxiv.org/abs/2409.11600](https://arxiv.org/abs/2409.11600)

Neve is a LLVM/C++ programming language. 

All the code is open sourced.


<div align="center">
  <img src="assets/neve_logo.png" alt="Logo" width="260" height="260">
</div>

Features: 
- Pythonic syntax;
- Compiled C++ FFI, and JIT for high-level;
- Close to C++ performance;
- Parallel coding with finish async expressions.
- Easy to read threads syntax that resemble go routines;
- Mark-sweep concurrent garbage collector with a go-like memory pool.

---

## Install



**Linux**

- Ubuntu 20.04.6 or higher
- WSL 2
- And other modern Linux distros

Install on ~/.local/neve
```bash
wget -qO- https://github.com/NoSavedDATA/Neve/releases/download/neve-bin/install.sh | bash
```
---

**Make**

- Requires a Linux distro;

- clang version 19;
  
```bash
apt-get install git make lsb-release wget software-properties-common gnupg
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
./llvm.sh 19 all

apt-get install llvm clang zlib1g-dev libzstd-dev libeigen3-dev libopencv-dev
```
- Add your local built `neve` to `PATH`, and set NEVE_LIBS env variable to the lib folder (not necessary for lib development, only if you want to Make Neve on your own).


- Make:
```bash
make -j8
```

This adds neve to bin/neve

