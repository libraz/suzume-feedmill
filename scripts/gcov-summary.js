#!/usr/bin/env node

/**
 * This script parses gcov files and displays a summary in the console,
 * similar to Jest's coverage report.
 */

const fs = require('fs');
const path = require('path');

// ANSI color codes for terminal output
const colors = {
  reset: '\x1b[0m',
  red: '\x1b[31m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  magenta: '\x1b[35m',
  cyan: '\x1b[36m',
  white: '\x1b[37m',
  gray: '\x1b[90m',
  bold: '\x1b[1m',
};

/**
 * Returns the appropriate color code based on coverage percentage
 * @param {number} percentage - Coverage percentage
 * @returns {string} ANSI color code
 */
function getColorForPercentage(percentage) {
  if (percentage >= 90) return colors.green;
  if (percentage >= 75) return colors.yellow;
  return colors.red;
}

/**
 * Recursively finds all .gcov files in the given directory
 * @param {string} dir - Directory to search
 * @returns {string[]} Array of file paths
 */
function findGcovFiles(dir) {
  const files = [];

  try {
    const entries = fs.readdirSync(dir, { withFileTypes: true });

    entries.forEach((entry) => {
      const fullPath = path.join(dir, entry.name);

      if (entry.isDirectory()) {
        files.push(...findGcovFiles(fullPath));
      } else if (entry.isFile() && entry.name.endsWith('.gcov')) {
        files.push(fullPath);
      }
    });
  } catch (err) {
    console.error(`Error reading directory ${dir}: ${err.message}`);
  }

  return files;
}

/**
 * Parses a .gcov file and extracts coverage information
 * @param {string} filePath - Path to the .gcov file
 * @returns {Object|null} Coverage statistics or null if parsing failed
 */
function parseGcovFile(filePath) {
  try {
    const content = fs.readFileSync(filePath, 'utf8');
    const lines = content.split('\n');

    // Extract source file name from the first line
    const sourceFileMatch = lines[0].match(/Source:(.+)$/);
    if (!sourceFileMatch) return null;

    const sourceFile = sourceFileMatch[1].trim();

    let fileTotalLines = 0;
    let fileCoveredLines = 0;
    let fileTotalFuncs = 0;
    let fileCoveredFuncs = 0;
    let fileTotalBranches = 0;
    let fileCoveredBranches = 0;
    const uncoveredLines = [];
    const functionMap = new Map();

    // Parse each line
    lines.forEach((line) => {
      // Function coverage
      if (line.includes('function ')) {
        const funcMatch = line.match(/function (.+) called (\d+) returned/);
        if (funcMatch) {
          const funcName = funcMatch[1].trim();
          const callCount = parseInt(funcMatch[2], 10);

          fileTotalFuncs += 1;
          if (callCount > 0) {
            fileCoveredFuncs += 1;
          }

          functionMap.set(funcName, callCount);
        }
      }

      // Line coverage
      const lineMatch = line.match(/^\s*([\d#-]+):/);
      if (lineMatch) {
        const coverage = lineMatch[1].trim();

        // Skip non-executable lines
        if (coverage === '-') return;

        // Count executable lines
        if (coverage !== '-' && coverage !== '=====') {
          fileTotalLines += 1;

          // Count covered lines
          if (coverage !== '#####' && coverage !== '=====') {
            fileCoveredLines += 1;
          } else {
            // Extract line number
            const lineNumMatch = line.match(/^\s*[\d#-]+:\s*(\d+):/);
            if (lineNumMatch) {
              uncoveredLines.push(lineNumMatch[1]);
            }
          }
        }
      }

      // Branch coverage (if available)
      if (line.includes('branch') && line.includes('taken')) {
        const branchMatch = line.match(/branch\s+\d+\s+(\w+)/);
        if (branchMatch) {
          fileTotalBranches += 1;
          if (branchMatch[1] === 'taken') {
            fileCoveredBranches += 1;
          }
        }
      }
    });

    return {
      file: sourceFile,
      lines: { total: fileTotalLines, covered: fileCoveredLines },
      functions: { total: fileTotalFuncs, covered: fileCoveredFuncs },
      branches: { total: fileTotalBranches, covered: fileCoveredBranches },
      uncoveredLines,
    };
  } catch (err) {
    console.error(`Error parsing ${filePath}: ${err.message}`);
    return null;
  }
}

// Find all .gcov files in the current directory
const currentDir = process.cwd();
const gcovFiles = findGcovFiles(currentDir);

if (gcovFiles.length === 0) {
  console.error(`${colors.red}No .gcov files found in ${currentDir}${colors.reset}`);
  console.error(
    `${colors.yellow}Run 'yarn test:cpp:coverage' first to generate coverage data.${colors.reset}`
  );
  process.exit(1);
}

console.log(`\n${colors.bold}${colors.cyan}Code Coverage Summary${colors.reset}\n`);

// Parse each .gcov file and collect statistics
const fileStats = [];
let totalLines = 0;
let totalCovered = 0;
let totalFunctions = 0;
let totalFunctionsCovered = 0;
let totalBranches = 0;
let totalBranchesCovered = 0;

gcovFiles.forEach((file) => {
  const stats = parseGcovFile(file);
  if (stats) {
    // Skip files with no executable lines
    if (stats.lines.total === 0) return;

    fileStats.push(stats);
    totalLines += stats.lines.total;
    totalCovered += stats.lines.covered;
    totalFunctions += stats.functions.total;
    totalFunctionsCovered += stats.functions.covered;
    totalBranches += stats.branches.total;
    totalBranchesCovered += stats.branches.covered;
  }
});

// Sort files by coverage percentage (ascending)
fileStats.sort((a, b) => {
  const aPct = a.lines.total > 0 ? a.lines.covered / a.lines.total : 0;
  const bPct = b.lines.total > 0 ? b.lines.covered / b.lines.total : 0;
  return aPct - bPct;
});

// Print table header
console.log(
  `${colors.bold}File                                 | % Stmts | % Branch | % Funcs | Uncovered Lines ${colors.reset}`
);
console.log(
  `${colors.gray}------------------------------------ | ------- | -------- | ------- | ---------------${colors.reset}`
);

// Print file statistics
fileStats.forEach((stats) => {
  const fileName = path.relative(process.cwd(), stats.file).replace(/\.gcov$/, '');
  const displayName = fileName.length > 35 ? `...${fileName.slice(-32)}` : fileName.padEnd(35);

  const linePct = stats.lines.total > 0 ? (stats.lines.covered / stats.lines.total) * 100 : 100;
  const branchPct =
    stats.branches.total > 0 ? (stats.branches.covered / stats.branches.total) * 100 : 100;
  const funcPct =
    stats.functions.total > 0 ? (stats.functions.covered / stats.functions.total) * 100 : 100;

  const lineColor = getColorForPercentage(linePct);
  const branchColor = getColorForPercentage(branchPct);
  const funcColor = getColorForPercentage(funcPct);

  console.log(
    `${displayName} | ${lineColor}${linePct.toFixed(1).padStart(6)}%${colors.reset} | ` +
      `${branchColor}${branchPct.toFixed(1).padStart(7)}%${colors.reset} | ` +
      `${funcColor}${funcPct.toFixed(1).padStart(6)}%${colors.reset} | ` +
      `${stats.uncoveredLines.slice(0, 5).join(', ')}${
        stats.uncoveredLines.length > 5 ? '...' : ''
      }`
  );
});

// Print totals
const totalLinePct = totalLines > 0 ? (totalCovered / totalLines) * 100 : 100;
const totalBranchPct = totalBranches > 0 ? (totalBranchesCovered / totalBranches) * 100 : 100;
const totalFuncPct = totalFunctions > 0 ? (totalFunctionsCovered / totalFunctions) * 100 : 100;

const totalLineColor = getColorForPercentage(totalLinePct);
const totalBranchColor = getColorForPercentage(totalBranchPct);
const totalFuncColor = getColorForPercentage(totalFuncPct);

console.log(
  `${colors.gray}------------------------------------ | ------- | -------- | ------- | ---------------${colors.reset}`
);
console.log(
  `${colors.bold}All files                          ${
    colors.reset
  } | ${totalLineColor}${totalLinePct.toFixed(1).padStart(6)}%${colors.reset} | ` +
    `${totalBranchColor}${totalBranchPct.toFixed(1).padStart(7)}%${colors.reset} | ` +
    `${totalFuncColor}${totalFuncPct.toFixed(1).padStart(6)}%${colors.reset} | `
);

console.log(`\n${colors.bold}Summary:${colors.reset}`);
console.log(
  `${colors.bold}Statements:${colors.reset} ${totalCovered}/${totalLines} (${totalLinePct.toFixed(
    2
  )}%)`
);
console.log(
  `${colors.bold}Branches:${
    colors.reset
  } ${totalBranchesCovered}/${totalBranches} (${totalBranchPct.toFixed(2)}%)`
);
console.log(
  `${colors.bold}Functions:${
    colors.reset
  } ${totalFunctionsCovered}/${totalFunctions} (${totalFuncPct.toFixed(2)}%)`
);

console.log(
  `\n${colors.gray}Coverage report generated from ${gcovFiles.length} .gcov files${colors.reset}`
);

// Helper functions section removed as functions are now defined at the top
