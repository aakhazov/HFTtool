# Install GCC and required libraries
sudo apt update
sudo apt install -y build-essential g++ autotools-dev libicu-dev libbz2-dev zlib1g-dev libxxf86vm-dev

# Install Boost
mkdir ~/cpp_workspace
cd ~/cpp_workspace
wget -O boost_1_79_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.79.0/boost_1_79_0.tar.gz/download
tar xzvf boost_1_79_0.tar.gz
cd ~/cpp_workspace/boost_1_79_0
./bootstrap.sh --prefix=/usr/local
./b2
sudo ./b2 install
cd ~/cpp_workspace
rm boost_1_79_0.tar.gz
sudo rm -rf boost_1_79_0

# Install OpenGL
sudo apt install -y libglfw3 libglfw3-dev freeglut3-dev

# Install OpenSSL
sudo apt install -y libssl-dev

# Install Git
sudo apt install -y git

# Install VS Code
cd ~/Downloads
wget -O vscode.deb 'https://code.visualstudio.com/sha/download?build=stable&os=linux-deb-x64'
sudo apt install -y ./vscode.deb

# Download HFTtool
cd ~/cpp_workspace
mkdir ~/cpp_workspace/_apps
cd ~/cpp_workspace/_apps
git clone https://github.com/aakhazov/HFTtool.git

# Build HFTtool
cd ~/cpp_workspace/_apps/HFTtool
make

# Launch HFTtool
./HFTtool

# Run VS Code
code
# Install extension: C/C++ Extension Pack
# Open ~/cpp_workspace/_apps/HFTtool folder in VS Code
# Ctrl+Shift+B -> select target


