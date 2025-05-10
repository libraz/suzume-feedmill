#!/usr/bin/env node

/**
 * This script generates an HTML coverage report for C++ code.
 * It handles platform-specific differences between macOS and Linux.
 */

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

// Check if running on Windows
if (process.platform === 'win32') {
  console.log('C++ coverage not supported on Windows.');
  console.log('JavaScript tests will still collect coverage information.');
  process.exit(0);
}

try {
  // Check if coverage data exists
  const profileDataFile = path.join('build', 'coverage.profdata');
  const coverageHtmlFile = path.join('build', 'coverage.html');
  const testExecutable = path.join('build', 'tests', 'suzume_feedmill_test');

  if (!fs.existsSync(profileDataFile)) {
    console.error(`Coverage data file not found: ${profileDataFile}`);
    console.log('Please run "yarn test:cpp:coverage" first to generate coverage data.');
    process.exit(1);
  }

  // Generate HTML report
  console.log('\nGenerating HTML coverage report...');

  if (process.platform === 'darwin') {
    // macOS uses xcrun
    execSync(
      `xcrun llvm-cov show ${testExecutable} -instr-profile=${profileDataFile} -show-line-counts-or-regions -show-expansions -format=html > ${coverageHtmlFile}`,
      { stdio: 'inherit' }
    );
  } else {
    // Linux uses direct commands
    execSync(
      `llvm-cov show ${testExecutable} -instr-profile=${profileDataFile} -show-line-counts-or-regions -show-expansions -format=html > ${coverageHtmlFile}`,
      { stdio: 'inherit' }
    );
  }

  console.log(`\nHTML coverage report generated at: ${coverageHtmlFile}`);
  console.log('You can open this file in a web browser to view the coverage report.');
} catch (error) {
  console.error(`Error generating HTML coverage report: ${error.message}`);
  process.exit(1);
}
