# Install GCC and required libraries
sudo dnf makecache --refresh
sudo dnf install -y xrandr
sudo dnf install -y libXxf86vm-devel
sudo dnf install -y make autoconf automake gcc gcc-c++ kernel-devel
sudo dnf install -y libicu-devel
sudo dnf install -y bzip2-libs
sudo dnf install -y zlib-devel
sudo dnf install -y libXxf86vm-devel

# Install Boost
wget -O boost_1_79_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.79.0/boost_1_79_0.tar.gz/download
tar xzvf boost_1_79_0.tar.gz
cd boost_1_79_0
./bootstrap.sh --prefix=/usr/local
./b2
sudo ./b2 install
cd ..
rm boost_1_79_0.tar.gz
sudo rm -rf boost_1_79_0

# Install OpenGL
sudo dnf install -y glfw-devel
sudo dnf install -y freeglut-devel

# Install OpenSSL
sudo dnf install -y openssl-devel

# Install Git
sudo dnf install -y git

# Install VS Code

# Build HFTtool
make -j$(nproc)

# Launch HFTtool
./HFTtool

# Run VS Code
code
# Install extension: C/C++ Extension Pack
# Open ~/cpp_workspace/_apps/HFTtool folder in VS Code
# Ctrl+Shift+B -> select target
