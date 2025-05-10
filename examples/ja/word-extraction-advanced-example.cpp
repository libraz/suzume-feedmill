#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * 日本語テキスト向け高度な単語抽出の例
 *
 * この例では以下を示します:
 * 1. 日本語テキストに最適化された高度なテキスト正規化
 * 2. 日本語の文字n-gramに対するPMI計算
 * 3. 日本語の未知語抽出と文脈分析
 * 4. 抽出された単語の後処理と分析
 */

// ファイルから行を読み込むヘルパー関数
std::vector<std::string> readLines(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        throw std::runtime_error("ファイルを開けませんでした: " + filename);
    }

    while (std::getline(file, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

// 行をファイルに書き込むヘルパー関数
void writeLines(const std::string& filename, const std::vector<std::string>& lines) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("書き込み用にファイルを開けませんでした: " + filename);
    }

    for (const auto& line : lines) {
        file << line << std::endl;
    }
}

// 抽出された単語のTSVファイルを解析するヘルパー関数
std::vector<std::tuple<std::string, double, int, bool>> parseWordsTsv(const std::string& filename) {
    std::vector<std::tuple<std::string, double, int, bool>> words;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        throw std::runtime_error("ファイルを開けませんでした: " + filename);
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::string word;
        double score = 0.0;
        int components = 0;
        bool verified = false;

        // TSV行を解析
        size_t pos1 = line.find('\t');
        if (pos1 != std::string::npos) {
            word = line.substr(0, pos1);

            size_t pos2 = line.find('\t', pos1 + 1);
            if (pos2 != std::string::npos) {
                score = std::stod(line.substr(pos1 + 1, pos2 - pos1 - 1));

                size_t pos3 = line.find('\t', pos2 + 1);
                if (pos3 != std::string::npos) {
                    components = std::stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
                    verified = line.substr(pos3 + 1) == "✓";
                }
            }
        }

        words.emplace_back(word, score, components, verified);
    }

    return words;
}

// 単語をフィルタリングして分類する後処理関数
void postProcessWords(const std::string& inputFile, const std::string& outputFile) {
    auto words = parseWordsTsv(inputFile);

    // スコアでソート
    std::sort(words.begin(), words.end(), [](const auto& a, const auto& b) {
        return std::get<1>(a) > std::get<1>(b);
    });

    // 長さで単語を分類
    std::map<int, std::vector<std::tuple<std::string, double, int, bool>>> wordsByLength;
    for (const auto& word : words) {
        int length = std::get<0>(word).length();
        wordsByLength[length].push_back(word);
    }

    // 分類結果を書き込み
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        throw std::runtime_error("書き込み用にファイルを開けませんでした: " + outputFile);
    }

    outFile << "# 高度な単語抽出結果" << std::endl;
    outFile << "# ---------------------------------" << std::endl;
    outFile << "# 抽出された単語の総数: " << words.size() << std::endl;
    outFile << std::endl;

    // 長さ別の要約を書き込み
    outFile << "## 長さ別の要約" << std::endl;
    outFile << "| 長さ | 数 | 平均スコア | 例 |" << std::endl;
    outFile << "|--------|-------|-----------|----------|" << std::endl;

    for (const auto& [length, lengthWords] : wordsByLength) {
        // 平均スコアを計算
        double totalScore = 0.0;
        for (const auto& word : lengthWords) {
            totalScore += std::get<1>(word);
        }
        double avgScore = totalScore / lengthWords.size();

        // 最大3つの例を取得
        std::string examples;
        for (size_t i = 0; i < std::min(size_t(3), lengthWords.size()); i++) {
            if (i > 0) examples += ", ";
            examples += std::get<0>(lengthWords[i]);
        }

        outFile << "| " << length << " | " << lengthWords.size() << " | "
                << std::fixed << std::setprecision(2) << avgScore << " | "
                << examples << " |" << std::endl;
    }

    outFile << std::endl;

    // スコア順の上位単語を書き込み
    outFile << "## スコア順の上位単語" << std::endl;
    outFile << "| 単語 | スコア | 構成要素数 | 検証 |" << std::endl;
    outFile << "|------|-------|------------|----------|" << std::endl;

    size_t topCount = std::min(size_t(50), words.size());
    for (size_t i = 0; i < topCount; i++) {
        const auto& [word, score, components, verified] = words[i];
        outFile << "| " << word << " | "
                << std::fixed << std::setprecision(2) << score << " | "
                << components << " | "
                << (verified ? "✓" : "") << " |" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // 引数をチェック
    if (argc < 2) {
        std::cerr << "使用法: " << argv[0] << " <入力テキストファイル> [出力ディレクトリ]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputDir = (argc > 2) ? argv[2] : "./output";

    // 出力ディレクトリが存在しない場合は作成
    fs::create_directories(outputDir);

    // 出力ファイルを定義
    std::string normalizedFile = outputDir + "/advanced-normalized.tsv";
    std::string pmiFile = outputDir + "/advanced-ngrams.tsv";
    std::string wordsFile = outputDir + "/advanced-words.tsv";
    std::string analysisFile = outputDir + "/advanced-analysis.md";
    std::string sampleFile = outputDir + "/advanced-sample.tsv";

    try {
        // 入力が大きすぎる場合はサンプルファイルを作成
        std::ifstream inFile(inputFile, std::ios::binary | std::ios::ate);
        std::streamsize size = inFile.tellg();
        inFile.close();

        if (size > 10 * 1024 * 1024) { // 10 MB
            std::cout << "入力ファイルが大きいです (" << (size / (1024 * 1024)) << " MB)、サンプルを作成しています..." << std::endl;

            auto lines = readLines(inputFile);
            size_t sampleSize = std::min(size_t(10000), lines.size());

            // ランダムに行を選択
            std::vector<std::string> sampleLines;
            std::sample(lines.begin(), lines.end(), std::back_inserter(sampleLines),
                       sampleSize, std::mt19937{std::random_device{}()});

            writeLines(sampleFile, sampleLines);
            inputFile = sampleFile;
            std::cout << sampleLines.size() << "行のサンプルを作成しました" << std::endl;
        }

        // ステップ1: 日本語テキスト向けの高度なオプションでテキストを正規化
        std::cout << "日本語テキスト向けの高度なオプションでテキストを正規化しています..." << std::endl;
        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC; // 日本語に最適
        normOpt.bloomFalsePositiveRate = 0.0001; // 高精度
        normOpt.threads = 8;
        normOpt.progressFormat = suzume::ProgressFormat::TTY;
        normOpt.progressCallback = [](double ratio) {
            std::cout << "\r正規化の進捗: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto normResult = suzume::normalize(inputFile, normalizedFile, normOpt);

        std::cout << "\n正規化が完了しました！" << std::endl;
        std::cout << normResult.rows << "行を処理し、"
                  << normResult.uniques << "行のユニークな結果を得ました。"
                  << normResult.duplicates << "行の重複が削除されました。" << std::endl;
        std::cout << "処理速度: " << normResult.mbPerSec << " MB/秒" << std::endl;

        // ステップ2: 日本語の文字n-gramに最適化されたパラメータでPMIを計算
        std::cout << "\n日本語の文字n-gramに最適化されたパラメータでPMIを計算しています..." << std::endl;
        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;  // 日本語の場合、2文字のn-gramが効果的
        pmiOpt.topK = 5000; // より多くの候補
        pmiOpt.minFreq = 2; // 低い閾値
        pmiOpt.threads = 8;
        pmiOpt.progressFormat = suzume::ProgressFormat::TTY;
        pmiOpt.progressCallback = [](double ratio) {
            std::cout << "\rPMI計算の進捗: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto pmiResult = suzume::calculatePmi(normalizedFile, pmiFile, pmiOpt);

        std::cout << "\nPMI計算が完了しました！" << std::endl;
        std::cout << pmiResult.grams << "個のn-gramを処理し、"
                  << pmiResult.distinctNgrams << "個の異なるn-gramを検出しました" << std::endl;
        std::cout << "処理速度: " << pmiResult.mbPerSec << " MB/秒" << std::endl;

        // ステップ3: 日本語の未知語抽出と文脈分析
        std::cout << "\n日本語の未知語抽出と文脈分析を実行しています..." << std::endl;
        suzume::WordExtractionOptions wordOpt;
        wordOpt.minPmiScore = 2.5; // 日本語の場合、より多くの候補を得るために低い閾値
        wordOpt.minLength = 2; // 日本語の場合、最小2文字
        wordOpt.maxLength = 15; // 日本語の複合語に対応するため長い単語も許可
        wordOpt.topK = 500; // より多くの結果
        wordOpt.verifyInOriginalText = true;
        wordOpt.useContextualAnalysis = true; // 日本語の文脈分析を有効化
        wordOpt.threads = 8;
        wordOpt.progressFormat = suzume::ProgressFormat::TTY;
        wordOpt.progressCallback = [](double ratio) {
            std::cout << "\r単語抽出の進捗: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto wordResult = suzume::extractWords(pmiFile, normalizedFile, wordsFile, wordOpt);

        std::cout << "\n単語抽出が完了しました！" << std::endl;
        std::cout << wordResult.words.size() << "個の潜在的な単語を抽出しました" << std::endl;
        std::cout << "処理時間: " << wordResult.processingTimeMs << " ミリ秒" << std::endl;

        // ステップ4: 結果の後処理と分析
        std::cout << "\n結果の後処理と分析を行っています..." << std::endl;
        postProcessWords(wordsFile, analysisFile);

        // 上位単語を表示
        std::cout << "\n抽出された上位単語:" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << std::left << std::setw(20) << "単語"
                  << std::setw(10) << "スコア"
                  << std::setw(10) << "頻度"
                  << "検証" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;

        size_t displayCount = std::min(size_t(10), wordResult.words.size());
        for (size_t i = 0; i < displayCount; i++) {
            std::cout << std::left << std::setw(20) << wordResult.words[i]
                      << std::setw(10) << std::fixed << std::setprecision(2) << wordResult.scores[i]
                      << std::setw(10) << wordResult.frequencies[i]
                      << (wordResult.verified[i] ? "✓" : "") << std::endl;
        }

        std::cout << "\n結果は以下に保存されました:" << std::endl;
        std::cout << "  - 正規化テキスト: " << normalizedFile << std::endl;
        std::cout << "  - PMI結果: " << pmiFile << std::endl;
        std::cout << "  - 抽出された単語: " << wordsFile << std::endl;
        std::cout << "  - 分析レポート: " << analysisFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
