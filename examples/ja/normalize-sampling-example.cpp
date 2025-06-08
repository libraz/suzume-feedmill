#include <suzume_feedmill.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * サンプリングの例
 *
 * この例では以下を示します:
 * 1. 組み込みのサンプリング機能の使用方法
 * 2. 大きなファイルの効率的な処理
 */
int main(int argc, char* argv[]) {
    // 引数をチェック
    if (argc < 3) {
        std::cerr << "使用法: " << argv[0] << " <入力ファイル> <出力ディレクトリ>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputDir = argv[2];

    // 出力ディレクトリが存在しない場合は作成
    fs::create_directories(outputDir);

    // 出力ファイルを定義
    std::string normalizedFile = outputDir + "/normalized.tsv";
    std::string sampleFile = outputDir + "/sample.tsv";

    try {
        // 入力ファイルのサイズをチェック
        std::ifstream inFile(inputFile, std::ios::binary | std::ios::ate);
        if (!inFile.is_open()) {
            throw std::runtime_error("入力ファイルを開けませんでした: " + inputFile);
        }
        std::streamsize size = inFile.tellg();
        inFile.close();

        std::cout << "入力ファイルのサイズ: " << (size / 1024.0 / 1024.0) << " MB" << std::endl;

        // 1000行のサンプルを作成
        std::cout << "\n1000行のサンプルを作成中..." << std::endl;

        suzume::NormalizeOptions sampleOpt;
        sampleOpt.form = suzume::NormalizationForm::NFKC;
        sampleOpt.progressFormat = suzume::ProgressFormat::TTY;
        sampleOpt.progressCallback = [](double ratio) {
            std::cout << "\rサンプリングの進捗: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        // 注: サンプルサイズは API で内部的に処理されます
        auto sampleResult = suzume::normalize(inputFile, sampleFile, sampleOpt);

        std::cout << "\nサンプリングが完了しました！" << std::endl;
        std::cout << sampleResult.rows << "行のサンプルを作成しました" << std::endl;
        std::cout << "サンプルファイルのサイズ: " << (fs::file_size(sampleFile) / 1024.0) << " KB" << std::endl;

        // 完全なファイルを処理
        std::cout << "\n完全なファイルを処理中..." << std::endl;

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
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

        // サンプルと完全なファイルの統計を比較
        std::cout << "\nサンプルと完全なファイルの比較:" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << std::left << std::setw(20) << "指標"
                  << std::setw(15) << "サンプル"
                  << "完全なファイル" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;

        std::cout << std::left << std::setw(20) << "行数"
                  << std::setw(15) << sampleResult.rows
                  << normResult.rows << std::endl;

        std::cout << std::left << std::setw(20) << "ユニーク行数"
                  << std::setw(15) << sampleResult.uniques
                  << normResult.uniques << std::endl;

        double sampleDupeRate = sampleResult.duplicates * 100.0 / sampleResult.rows;
        double fullDupeRate = normResult.duplicates * 100.0 / normResult.rows;

        std::cout << std::left << std::setw(20) << "重複率"
                  << std::setw(15) << std::fixed << std::setprecision(2) << sampleDupeRate << "%"
                  << std::fixed << std::setprecision(2) << fullDupeRate << "%" << std::endl;

        std::cout << "\n結果は以下に保存されました:" << std::endl;
        std::cout << "  - サンプルファイル: " << sampleFile << std::endl;
        std::cout << "  - 完全な正規化ファイル: " << normalizedFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
