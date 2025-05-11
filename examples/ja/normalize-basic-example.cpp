#include <suzume_feedmill.h>
#include <iostream>
#include <iomanip>

/**
 * 基本的な正規化の例
 *
 * この例では以下を示します:
 * 1. 基本的なテキスト正規化
 * 2. 行長フィルタの使用方法
 */
int main(int argc, char* argv[]) {
    // 引数をチェック
    if (argc < 3) {
        std::cerr << "使用法: " << argv[0] << " <入力ファイル> <出力ファイル>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    try {
        // 基本的な正規化
        std::cout << "基本的な正規化を実行中..." << std::endl;

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 4;
        normOpt.progressFormat = suzume::ProgressFormat::TTY;
        normOpt.progressCallback = [](double ratio) {
            std::cout << "\r正規化の進捗: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto result = suzume::normalize(inputFile, outputFile, normOpt);

        std::cout << "\n正規化が完了しました！" << std::endl;
        std::cout << result.rows << "行を処理し、"
                  << result.uniques << "行のユニークな結果を得ました。"
                  << result.duplicates << "行の重複が削除されました。" << std::endl;
        std::cout << "処理速度: " << result.mbPerSec << " MB/秒" << std::endl;

        // 行長フィルタを使用した正規化
        std::string filteredOutput = outputFile + ".filtered";
        std::cout << "\n行長フィルタを使用した正規化を実行中..." << std::endl;

        suzume::NormalizeOptions filterOpt;
        filterOpt.form = suzume::NormalizationForm::NFKC;
        filterOpt.threads = 4;
        filterOpt.minLength = 10;  // 10文字未満の行をフィルタリング
        filterOpt.maxLength = 200; // 200文字を超える行をフィルタリング
        filterOpt.progressFormat = suzume::ProgressFormat::TTY;
        filterOpt.progressCallback = [](double ratio) {
            std::cout << "\rフィルタリングの進捗: " << std::fixed << std::setprecision(1)
                      << (ratio * 100.0) << "%" << std::flush;
        };

        auto filteredResult = suzume::normalize(inputFile, filteredOutput, filterOpt);

        std::cout << "\nフィルタリングが完了しました！" << std::endl;
        std::cout << filteredResult.rows << "行を処理し、"
                  << filteredResult.uniques << "行のユニークな結果を得ました。"
                  << filteredResult.duplicates << "行の重複が削除されました。" << std::endl;
        std::cout << "行長でフィルタリングされた行数: "
                  << (result.rows - filteredResult.rows) << std::endl;
        std::cout << "処理速度: " << filteredResult.mbPerSec << " MB/秒" << std::endl;

        std::cout << "\n結果は以下に保存されました:" << std::endl;
        std::cout << "  - 基本的な正規化: " << outputFile << std::endl;
        std::cout << "  - フィルタリングされた正規化: " << filteredOutput << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
