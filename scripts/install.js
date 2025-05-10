#!/usr/bin/env node
/* eslint-disable @typescript-eslint/no-var-requires */
/**
 * Installation script for suzume-feedmill
 * Builds and installs the CLI binary and Node.js binding
 */
const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

// Root directory
const rootDir = path.resolve(__dirname, '..');
const binDir = path.join(rootDir, 'bin');

// Platform information
const isWin = process.platform === 'win32';
const finalBinName = 'suzume-feedmill';

// Create directory if it doesn't exist
if (!fs.existsSync(binDir)) {
  fs.mkdirSync(binDir, { recursive: true });
}

/**
 * Check if required build dependencies are installed
 * @returns {void}
 */
function checkDependencies() {
  const dependencies = [
    { name: 'CMake', command: 'cmake --version' },
    { name: 'C++ Compiler', command: process.platform === 'win32' ? 'cl' : 'c++ --version' },
  ];

  const missing = [];

  dependencies.forEach((dep) => {
    try {
      execSync(dep.command, { stdio: 'ignore' });
    } catch (error) {
      missing.push(dep.name);
    }
  });

  if (missing.length > 0) {
    console.error(`Missing dependencies: ${missing.join(', ')}`);
    console.error('Please install the required dependencies and try again');

    if (missing.includes('CMake')) {
      console.error('CMake installation instructions: https://cmake.org/install/');
    }

    if (missing.includes('C++ Compiler')) {
      if (process.platform === 'win32') {
        console.error('Visual Studio installation: https://visualstudio.microsoft.com/');
      } else if (process.platform === 'darwin') {
        console.error('Install Xcode Command Line Tools: xcode-select --install');
      } else {
        console.error('Install GCC: sudo apt-get install build-essential');
      }
    }

    process.exit(1);
  }
}

// Check dependencies before proceeding
checkDependencies();

// Build the CLI binary
try {
  console.log('Building suzume-feedmill CLI...');

  // Check if we're on Windows and ICU might be missing
  if (isWin && !process.env.CMAKE_TOOLCHAIN_FILE && !process.env.VCPKG_INSTALLATION_ROOT) {
    console.warn('WARNING: Running on Windows without ICU configuration.');
    console.warn('Will attempt to build, but may fail if ICU libraries are not found.');
  }

  // Check if we're on Windows and have a toolchain file
  let cmakeCommand = 'cmake -B build -DBUILD_CLI=ON';
  if (isWin) {
    if (process.env.CMAKE_TOOLCHAIN_FILE) {
      console.log(`Using toolchain file: ${process.env.CMAKE_TOOLCHAIN_FILE}`);
      cmakeCommand += ` -DCMAKE_TOOLCHAIN_FILE="${process.env.CMAKE_TOOLCHAIN_FILE}"`;
    } else if (process.env.VCPKG_INSTALLATION_ROOT) {
      const toolchainPath = `${process.env.VCPKG_INSTALLATION_ROOT}\\scripts\\buildsystems\\vcpkg.cmake`;
      console.log(`Using vcpkg toolchain file: ${toolchainPath}`);
      cmakeCommand += ` -DCMAKE_TOOLCHAIN_FILE="${toolchainPath}"`;
    } else {
      console.warn('WARNING: No CMAKE_TOOLCHAIN_FILE or VCPKG_INSTALLATION_ROOT found.');
      console.warn('ICU libraries may not be found. Consider installing vcpkg and ICU:');
      console.warn('  vcpkg install icu:x64-windows');
      console.warn(
        '  setx CMAKE_TOOLCHAIN_FILE %VCPKG_INSTALLATION_ROOT%\\scripts\\buildsystems\\vcpkg.cmake'
      );
    }
  }

  execSync(cmakeCommand, { stdio: 'inherit', cwd: rootDir });

  // Build the CLI target
  console.log('Building CLI target...');
  execSync('cmake --build build --config Release', {
    stdio: 'inherit',
    cwd: rootDir,
  });

  // Initial build completed

  // Build Node.js binding using npx to ensure command availability
  console.log('Building Node.js binding...');
  try {
    execSync('npx node-gyp rebuild --jobs max', {
      stdio: 'inherit',
      cwd: rootDir,
    });
  } catch (bindingError) {
    console.error('Error building Node.js binding:', bindingError.message);
    console.error('Continuing with installation, but Node.js binding may not work properly.');
  }

  // Build TypeScript files using npx
  console.log('Building TypeScript files...');
  try {
    execSync('npx tsc -p tsconfig.build.cjs.json', {
      stdio: 'inherit',
      cwd: rootDir,
    });
    execSync('npx tsc -p tsconfig.build.esm.json', {
      stdio: 'inherit',
      cwd: rootDir,
    });
  } catch (tsError) {
    console.error('Error building TypeScript files:', tsError.message);
    console.error(
      'Continuing with installation, but TypeScript files may not be properly compiled.'
    );
  }
} catch (error) {
  console.error('Error building suzume-feedmill:', error.message);

  // Check if the error is related to ICU libraries
  if (error.message.includes('ICU libraries not found')) {
    console.warn('');
    console.warn('ICU libraries not found. This is required for Unicode support.');
    console.warn('');
    console.warn('On Windows, you can install ICU with vcpkg:');
    console.warn('  1. vcpkg install icu:x64-windows');
    console.warn(
      '  2. setx CMAKE_TOOLCHAIN_FILE %VCPKG_INSTALLATION_ROOT%\\scripts\\buildsystems\\vcpkg.cmake'
    );
    console.warn('');
    console.warn('On macOS:');
    console.warn('  brew install icu4c');
    console.warn('');
    console.warn('On Linux:');
    console.warn('  sudo apt-get install libicu-dev');
    console.warn('');
    console.warn('Continuing installation without native components...');

    // Create a minimal JavaScript wrapper that explains the missing native module
    const jsWrapperPath = path.join(binDir, 'suzume-feedmill.js');
    const jsWrapperContent = `#!/usr/bin/env node
console.error('Error: Native suzume-feedmill module could not be built.');
console.error('ICU libraries are required but were not found during installation.');
console.error('');
console.error('Please install ICU and rebuild:');
console.error('  Windows: vcpkg install icu:x64-windows');
console.error('  macOS: brew install icu4c');
console.error('  Linux: sudo apt-get install libicu-dev');
console.error('');
console.error('Then run: npm rebuild');
process.exit(1);
`;
    fs.writeFileSync(jsWrapperPath, jsWrapperContent);
    fs.chmodSync(jsWrapperPath, '755');

    // Continue with installation of other components
    return;
  }

  process.exit(1);
}

// Copy the binary to the bin directory
try {
  console.log('Installing suzume-feedmill binary...');

  // Force rebuild the CLI binary to ensure it exists
  console.log('Rebuilding CLI binary...');
  try {
    // First try to build with existing cache
    execSync('cmake --build build --target suzume_feedmill_cli --config Release', {
      stdio: 'inherit',
      cwd: rootDir,
    });
  } catch (buildError) {
    console.log('Error building with existing cache, regenerating...');
    // If that fails, regenerate the cache and try again

    // Check if we're on Windows and have a toolchain file
    let cmakeCommand = 'cmake -B build -DBUILD_CLI=ON';
    if (isWin) {
      if (process.env.CMAKE_TOOLCHAIN_FILE) {
        console.log(`Using toolchain file: ${process.env.CMAKE_TOOLCHAIN_FILE}`);
        cmakeCommand += ` -DCMAKE_TOOLCHAIN_FILE="${process.env.CMAKE_TOOLCHAIN_FILE}"`;
      } else if (process.env.VCPKG_INSTALLATION_ROOT) {
        const toolchainPath = `${process.env.VCPKG_INSTALLATION_ROOT}\\scripts\\buildsystems\\vcpkg.cmake`;
        console.log(`Using vcpkg toolchain file: ${toolchainPath}`);
        cmakeCommand += ` -DCMAKE_TOOLCHAIN_FILE="${toolchainPath}"`;
      } else {
        console.warn('WARNING: No CMAKE_TOOLCHAIN_FILE or VCPKG_INSTALLATION_ROOT found.');
        console.warn('ICU libraries may not be found. Consider installing vcpkg and ICU:');
        console.warn('  vcpkg install icu:x64-windows');
        console.warn(
          '  setx CMAKE_TOOLCHAIN_FILE %VCPKG_INSTALLATION_ROOT%\\scripts\\buildsystems\\vcpkg.cmake'
        );
      }
    }

    execSync(cmakeCommand, {
      stdio: 'inherit',
      cwd: rootDir,
    });
    execSync('cmake --build build --target suzume_feedmill_cli --config Release', {
      stdio: 'inherit',
      cwd: rootDir,
    });
  }

  // Check if the binary exists after rebuild
  const directPath = path.join(rootDir, 'build', 'suzume-feedmill');

  // Check if the binary exists
  let binPath = null;
  if (fs.existsSync(directPath)) {
    binPath = directPath;
  } else {
    console.error('Error: Binary not found after rebuild');
    process.exit(1);
  }

  // Copy to bin directory
  const destPath = path.join(binDir, finalBinName);
  fs.copyFileSync(binPath, destPath);

  // Set executable permissions on Unix-like systems
  if (!isWin) {
    fs.chmodSync(destPath, '755');
  }

  console.log(`Binary installed at ${destPath}`);

  // Create .cmd file for Windows
  if (isWin) {
    const cmdPath = path.join(binDir, 'suzume-feedmill.cmd');
    fs.writeFileSync(cmdPath, `@echo off\r\n"%~dp0\\suzume-feedmill" %*`);
    console.log(`Command wrapper created at ${cmdPath}`);
  }
} catch (error) {
  console.error('Error installing binary:', error.message);
  process.exit(1);
}

console.log('Installation completed successfully');
