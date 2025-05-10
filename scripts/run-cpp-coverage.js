#!/usr/bin/env node

/**
 * This script runs C++ code coverage tests.
 * It handles platform-specific differences between macOS and Linux.
 */

const { execSync } = require('child_process');
const path = require('path');

// Check if running on Windows
if (process.platform === 'win32') {
  console.log('C++ coverage not supported on Windows.');
  console.log('JavaScript tests will still collect coverage information.');
  process.exit(0);
}

try {
  // Run the coverage tools check first
  execSync('node scripts/check-coverage-tools.js', { stdio: 'inherit' });

  // Build with coverage enabled
  console.log('\nBuilding with coverage enabled...');
  execSync('cmake -B build -DENABLE_COVERAGE=ON', { stdio: 'inherit' });
  execSync('cmake --build build --target suzume_feedmill_test --parallel', { stdio: 'inherit' });

  // Run the tests with coverage
  console.log('\nRunning tests with coverage...');
  const profileFile = path.join('build', 'coverage.profraw');
  const profileDataFile = path.join('build', 'coverage.profdata');
  const coverageTextFile = path.join('build', 'coverage.txt');
  const testExecutable = path.join('build', 'tests', 'suzume_feedmill_test');

  // Set environment variable for profile output
  process.env.LLVM_PROFILE_FILE = profileFile;

  // Run the test executable
  execSync(`./${testExecutable}`, { stdio: 'inherit' });

  // Process coverage data
  console.log('\nProcessing coverage data...');

  if (process.platform === 'darwin') {
    // macOS uses xcrun
    execSync(`xcrun llvm-profdata merge -sparse ${profileFile} -o ${profileDataFile}`, {
      stdio: 'inherit',
    });
    execSync(
      `xcrun llvm-cov show ${testExecutable} -instr-profile=${profileDataFile} -show-line-counts-or-regions -show-expansions -format=text > ${coverageTextFile}`,
      { stdio: 'inherit' }
    );
    execSync(`xcrun llvm-cov report ${testExecutable} -instr-profile=${profileDataFile}`, {
      stdio: 'inherit',
    });
  } else {
    // Linux uses direct commands
    execSync(`llvm-profdata merge -sparse ${profileFile} -o ${profileDataFile}`, {
      stdio: 'inherit',
    });
    execSync(
      `llvm-cov show ${testExecutable} -instr-profile=${profileDataFile} -show-line-counts-or-regions -show-expansions -format=text > ${coverageTextFile}`,
      { stdio: 'inherit' }
    );
    execSync(`llvm-cov report ${testExecutable} -instr-profile=${profileDataFile}`, {
      stdio: 'inherit',
    });
  }

  console.log('\nCoverage data generated successfully.');
  console.log(`Coverage text report: ${coverageTextFile}`);
  console.log('Run "yarn test:cpp:coverage:summary" to see a summary of the coverage.');
  console.log('Run "yarn test:cpp:coverage:html" to generate an HTML coverage report.');
} catch (error) {
  console.error(`Error running C++ coverage: ${error.message}`);
  process.exit(1);
}
