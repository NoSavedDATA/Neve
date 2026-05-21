# Neve Compiler

Documentation: [https://nsk-lang.dev](https://nsk-lang.dev)

Official repository of: [https://arxiv.org/abs/2409.11600](https://arxiv.org/abs/2409.11600)

Neve is a LLVM/C++ programming language. 

All the code is open sourced.


<div align="center">
  <img src="assets/neve_logo.png" alt="Logo" width="260" height="260">
</div>

Features: 
- Parallel coding with finish async expressions.
- Pythonic syntax;
- Object Oriented (no inheritance, just composition);
- The low-level background C++ functions are compiled;
- High-level uses Just-in-Time Compilation;
- Close to C++ performance;
- Easy to read threads syntax that resemble go routines;
- Mark-sweep garbage collector with a go-like memory pool;
- Ease of extending it with C++ functions.

---

## Install



**Ubuntu**

- WSL 2 or Ubuntu 20.04.6 or higher

Install on /usr/bin/neve
```bash
wget https://github.com/NoSavedDATA/Neve/releases/download/neve-bin/install.sh
chmod +x install.sh
./install.sh 
source ~/.bashrc
```

Install on ~/.local/neve
```bash
wget https://github.com/NoSavedDATA/Neve/releases/download/neve-bin/install.sh
chmod +x install.sh
./install.sh 
source ~/.bashrc
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
- Add commands `neve` to `PATH`:

```bash
chmod +x alias.sh
./alias.sh
source ~/.bashsr
```

- Make:

```bash
make -j8
```

This adds neve to bin/neve

