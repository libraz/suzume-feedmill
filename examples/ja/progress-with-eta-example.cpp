#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <thread>
#include <vector>
#include <random>

/**
 * ETA付き進捗バーの例
 *
 * この例では以下を示します:
 * 1. ETA機能付き進捗バーの使用方法
 * 2. 進捗表示のカスタマイズ
 * 3. 異なる進捗フォーマットの比較
 */

// 大きなテストファイルを生成するヘルパー関数
void generateTestFile(const std::string& filename, size_t lineCount) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("書き込み用にファイルを開けませんでした: " + filename);
    }

    std::cout << lineCount << "行のテストファイルを生成中..." << std::endl;

    // テキストのバリエーション用の乱数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> wordCount(5, 20);
    std::uniform_int_distribution<> wordLength(3, 12);
    std::uniform_int_distribution<> charDist(97, 122); // a-z

    for (size_t i = 0; i < lineCount; i++) {
        // ランダムな行を生成
        int words = wordCount(gen);
        for (int w = 0; w < words; w++) {
            int len = wordLength(gen);
            for (int c = 0; c < len; c++) {
                file << static_cast<char>(charDist(gen));
            }
            file << " ";
        }
        file << "\n";

        // 進捗を表示
        if (i % 1000 == 0) {
            std::cout << "\r" << i << "行を生成しました..." << std::flush;
        }
    }

    file.close();
    std::cout << "\r" << lineCount << "行を生成しました。                " << std::endl;
}

// 手動のETA計算を含むカスタム進捗コールバック
void customProgressCallback(double ratio) {
    static auto startTime = std::chrono::steady_clock::now();
    static int lastPercent = -1;

    int percent = static_cast<int>(ratio * 100);

    if (percent != lastPercent) {
        auto now = std::chrono::steady_clock::now();
        double elapsedSeconds = std::chrono::duration<double>(now - startTime).count();

        // ETAを計算
        double eta = 0.0;
        if (ratio > 0.0 && ratio < 1.0) {
            double totalEstimatedTime = elapsedSeconds / ratio;
            eta = totalEstimatedTime - elapsedSeconds;
        }

        // ETA文字列をフォーマット
        std::string etaStr;
        if (ratio <= 0.0 || ratio >= 1.0) {
            etaStr = "";
        } else {
            int etaMinutes = static_cast<int>(eta) / 60;
            int etaSeconds = static_cast<int>(eta) % 60;

            std::stringstream ss;
            ss << " 残り時間: ";
            if (etaMinutes > 0) {
                ss << etaMinutes << "分 ";
            }
            ss << etaSeconds << "秒";
            etaStr = ss.str();
        }

        // 進捗バーを作成
        const int barWidth = 40;
        int pos = barWidth * ratio;

        std::cout << "\r[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }

        std::cout << "] " << percent << "%" << etaStr << std::flush;
        lastPercent = percent;

        if (percent >= 100) {
            std::cout << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    // 例のためのファイルパスを定義
    std::string tempDir = std::filesystem::temp_directory_path().string();
    std::string testFile = tempDir + "/eta_test_data.txt";
    std::string outputDir = tempDir + "/eta_test_output";

    // 出力ディレクトリを作成
    std::filesystem::create_directories(outputDir);

    try {
        // テストファイルを生成
        generateTestFile(testFile, 50000);

        // 例1: 組み込みのETA進捗表示を使用（TTYフォーマット）
        std::cout << "\n例1: 組み込みのETA進捗表示（TTYフォーマット）" << std::endl;

        suzume::NormalizeOptions ttyOpt;
        ttyOpt.form = suzume::NormalizationForm::NFKC;
        ttyOpt.threads = 4;
        ttyOpt.progressFormat = suzume::ProgressFormat::TTY;
        // ETA付きの進捗コールバックが自動的に使用されます

        std::string ttyOutput = outputDir + "/tty_output.tsv";
        auto ttyResult = suzume::normalize(testFile, ttyOutput, ttyOpt);

        std::cout << "TTYフォーマット処理が完了しました！" << std::endl;
        std::cout << ttyResult.rows << "行を"
                  << ttyResult.elapsedMs << "ミリ秒で処理しました" << std::endl;

        // 例2: ETA付きのJSON進捗フォーマットを使用
        std::cout << "\n例2: ETA付きのJSON進捗フォーマット" << std::endl;

        suzume::NormalizeOptions jsonOpt;
        jsonOpt.form = suzume::NormalizationForm::NFKC;
        jsonOpt.threads = 4;
        jsonOpt.progressFormat = suzume::ProgressFormat::JSON;

        std::string jsonOutput = outputDir + "/json_output.tsv";

        std::cout << "JSON進捗出力（最初の数行）:" << std::endl;
        // いくつかの進捗更新をキャプチャ
        int captureCount = 0;
        jsonOpt.progressCallback = [&captureCount](double ratio) {
            if (captureCount < 5) {
                // 実際のシナリオでは、これはstderrに送信されます
                std::cout << "{\"progress\":" << static_cast<int>(ratio * 100)
                          << ", \"eta\":" << (1.0 - ratio) * 30 << "}" << std::endl;
                captureCount++;
            }
        };

        auto jsonResult = suzume::normalize(testFile, jsonOutput, jsonOpt);

        std::cout << "JSONフォーマット処理が完了しました！" << std::endl;
        std::cout << jsonResult.rows << "行を"
                  << jsonResult.elapsedMs << "ミリ秒で処理しました" << std::endl;

        // 例3: ETA付きのカスタム進捗表示
        std::cout << "\n例3: ETA付きのカスタム進捗表示" << std::endl;

        suzume::NormalizeOptions customOpt;
        customOpt.form = suzume::NormalizationForm::NFKC;
        customOpt.threads = 4;
        customOpt.progressCallback = customProgressCallback;

        std::string customOutput = outputDir + "/custom_output.tsv";
        auto customResult = suzume::normalize(testFile, customOutput, customOpt);

        std::cout << "カスタムフォーマット処理が完了しました！" << std::endl;
        std::cout << customResult.rows << "行を"
                  << customResult.elapsedMs << "ミリ秒で処理しました" << std::endl;

        // クリーンアップ
        std::cout << "\n一時ファイルをクリーンアップ中..." << std::endl;
        std::filesystem::remove(testFile);
        std::filesystem::remove_all(outputDir);

        std::cout << "クリーンアップが完了しました！" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
