#!/usr/bin/env node

/**
 * This script checks if llvm-cov is installed for code coverage.
 */

const { execSync } = require('child_process');
const chalk = require('chalk') || { green: (s) => s, red: (s) => s, yellow: (s) => s };

console.log('Checking for code coverage tools...');

// Check if running on Windows
if (process.platform === 'win32') {
  console.log(chalk.yellow('Code coverage for C++ is not supported on Windows.'));
  console.log(chalk.yellow('JavaScript tests will still collect coverage information.'));
  process.exit(0);
}

const checkCommand = (command, name, installInstructions) => {
  try {
    execSync(command, { stdio: 'ignore' });
    console.log(chalk.green(`✓ ${name} is installed`));
    return true;
  } catch (error) {
    console.log(chalk.red(`✗ ${name} is not installed`));
    console.log(chalk.yellow(`  To install ${name}:`));
    console.log(installInstructions);
    return false;
  }
};

// Check for coverage tools based on platform
let isLlvmCovInstalled = false;
let isLlvmProfdataInstalled = false;

if (process.platform === 'darwin') {
  // On macOS, we use xcrun to find llvm-cov
  isLlvmCovInstalled = checkCommand(
    'xcrun --find llvm-cov',
    'llvm-cov',
    `  macOS: Install Xcode Command Line Tools (xcode-select --install)`
  );

  isLlvmProfdataInstalled = checkCommand(
    'xcrun --find llvm-profdata',
    'llvm-profdata',
    `  macOS: Install Xcode Command Line Tools (xcode-select --install)`
  );
} else {
  // On Linux, we check directly
  isLlvmCovInstalled = checkCommand(
    'which llvm-cov',
    'llvm-cov',
    `  Ubuntu: sudo apt-get install llvm
  Other: Install LLVM toolchain`
  );

  isLlvmProfdataInstalled = checkCommand(
    'which llvm-profdata',
    'llvm-profdata',
    `  Ubuntu: sudo apt-get install llvm
  Other: Install LLVM toolchain`
  );
}

if (!isLlvmCovInstalled || !isLlvmProfdataInstalled) {
  console.log(
    chalk.yellow(
      '\nSome LLVM coverage tools are missing. Please install them before running coverage tests.'
    )
  );
  process.exit(1);
} else {
  console.log(
    chalk.green('\nAll required LLVM coverage tools are installed. You can run coverage tests.')
  );

  // Check llvm-cov version
  try {
    const versionCommand =
      process.platform === 'darwin' ? 'xcrun llvm-cov --version' : 'llvm-cov --version';
    const llvmCovVersion = execSync(versionCommand, { encoding: 'utf8' });
    console.log(chalk.green(`llvm-cov version: ${llvmCovVersion.split('\n')[0]}`));
  } catch (error) {
    console.log(chalk.yellow('Could not determine llvm-cov version.'));
  }
}
