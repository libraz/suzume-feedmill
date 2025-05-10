/* eslint-disable @typescript-eslint/no-require-imports */
/**
 * Script to install build tools required for native compilation
 */

const { execSync } = require('child_process');
const os = require('os');
const fs = require('fs');
const path = require('path');

// Log with timestamp
function log(message) {
  const timestamp = new Date().toISOString();
  console.log(`[${timestamp}] ${message}`);
}

// Execute command and log output
function execute(command, options = {}) {
  log(`Executing: ${command}`);
  try {
    const output = execSync(command, {
      stdio: 'pipe',
      encoding: 'utf8',
      ...options,
    });
    log('Command completed successfully');
    return output.trim();
  } catch (error) {
    log(`Command failed with exit code ${error.status}`);
    log(`Error output: ${error.stderr}`);
    throw error;
  }
}

// Check if a command exists
function commandExists(command) {
  try {
    execSync(os.platform() === 'win32' ? `where ${command}` : `which ${command}`, {
      stdio: 'ignore',
    });
    return true;
  } catch {
    // エラーオブジェクトは使用しないため、パラメータを省略
    return false;
  }
}

// Install build tools on Linux
function installLinuxBuildTools() {
  // Check for package manager
  if (commandExists('apt-get')) {
    log('Using apt-get to install build tools');
    execute('apt-get update -y');
    execute('apt-get install -y build-essential cmake');
  } else if (commandExists('yum')) {
    log('Using yum to install build tools');
    execute('yum groupinstall -y "Development Tools"');
    execute('yum install -y cmake');
  } else if (commandExists('dnf')) {
    log('Using dnf to install build tools');
    execute('dnf groupinstall -y "Development Tools"');
    execute('dnf install -y cmake');
  } else if (commandExists('pacman')) {
    log('Using pacman to install build tools');
    execute('pacman -Sy --noconfirm base-devel cmake');
  } else {
    log('No supported package manager found');
    log('Please install build-essential and cmake manually');
  }
}

// Install build tools on macOS
function installMacOSBuildTools() {
  // Check for Homebrew
  if (commandExists('brew')) {
    log('Using Homebrew to install build tools');
    execute('brew install cmake');
  } else {
    log('Homebrew not found');
    log('Installing Homebrew...');
    execute(
      '/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
    );
    log('Installing cmake...');
    execute('brew install cmake');
  }

  // Check for Xcode Command Line Tools
  try {
    execute('xcode-select --print-path', { stdio: 'ignore' });
    log('Xcode Command Line Tools already installed');
  } catch {
    // エラーオブジェクトは使用しないため、パラメータを省略
    log('Installing Xcode Command Line Tools...');
    execute('xcode-select --install');
    log('Please complete the installation of Xcode Command Line Tools and run this script again');
    process.exit(1);
  }
}

// Install build tools on Windows
function installWindowsBuildTools() {
  // Check for Visual Studio
  const programFiles = process.env.ProgramFiles || process.env['ProgramFiles(x86)'];
  const vsPath = path.join(programFiles, 'Microsoft Visual Studio');

  if (fs.existsSync(vsPath)) {
    log('Visual Studio found');
  } else {
    log('Visual Studio not found');
    log('Please install Visual Studio with C++ development tools');
    log('https://visualstudio.microsoft.com/downloads/');
  }

  // Check for CMake
  if (commandExists('cmake')) {
    log('CMake found');
  } else {
    log('CMake not found');
    log('Installing CMake...');
    execute('npm install -g cmake-js');
  }
}

// Install build tools based on platform
function installBuildTools() {
  const platform = os.platform();

  log(`Detected platform: ${platform}`);

  if (platform === 'linux') {
    installLinuxBuildTools();
  } else if (platform === 'darwin') {
    installMacOSBuildTools();
  } else if (platform === 'win32') {
    installWindowsBuildTools();
  } else {
    log(`Unsupported platform: ${platform}`);
    process.exit(1);
  }
}

// Main function
function main() {
  try {
    log('Starting build tools installation');
    installBuildTools();
    log('Build tools installation completed');
  } catch (error) {
    log(`Error: ${error.message}`);
    process.exit(1);
  }
}

// Run main function
main();
