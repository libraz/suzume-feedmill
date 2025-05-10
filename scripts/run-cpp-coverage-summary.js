#!/usr/bin/env node

/**
 * This script displays a summary of C++ code coverage.
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
  // Display summary
  console.log('\nC++ Code Coverage Summary:');
  console.log('==========================\n');

  // Use the llvm-cov report command directly to get the summary
  try {
    console.log('Generating coverage summary...');

    const testExecutable = path.join('build', 'tests', 'suzume_feedmill_test');
    const profileDataFile = path.join('build', 'coverage.profdata');

    if (!fs.existsSync(profileDataFile)) {
      console.error(`Coverage data file not found: ${profileDataFile}`);
      console.log('Please run "yarn test:cpp:coverage" first to generate coverage data.');
      process.exit(1);
    }

    let command;
    if (process.platform === 'darwin') {
      command = `xcrun llvm-cov report ${testExecutable} -instr-profile=${profileDataFile}`;
    } else {
      command = `llvm-cov report ${testExecutable} -instr-profile=${profileDataFile}`;
    }

    const result = execSync(command, { encoding: 'utf8' });
    console.log(result);
  } catch (error) {
    console.error(`Error generating coverage summary: ${error.message}`);
    console.log('No summary information found in the coverage report.');
  }

  console.log('\nFor detailed coverage information:');
  console.log(`- View the full text report: ${path.join('build', 'coverage.txt')}`);
  console.log('- Run "yarn test:cpp:coverage:html" to generate an HTML report');
} catch (error) {
  console.error(`Error displaying coverage summary: ${error.message}`);
  process.exit(1);
}
