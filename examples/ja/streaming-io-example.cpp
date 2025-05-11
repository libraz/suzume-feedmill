#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

/**
 * ストリーミングI/Oの例
 *
 * この例では以下を示します:
 * 1. stdin/stdoutを使用したストリーミング処理
 * 2. suzume-feedmillを使用したパイプライン処理の作成
 * 3. データストリーミングシナリオのシミュレーション
 */

// データストリームをシミュレートするヘルパー関数
void simulateDataStream(const std::string& outputFile, int lineCount, int delayMs) {
    std::ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        throw std::runtime_error("出力ファイルを開けませんでした: " + outputFile);
    }

    std::cout << outputFile << "へのデータストリームをシミュレート中..." << std::endl;

    for (int i = 0; i < lineCount; i++) {
        // バリエーションのある行を生成
        outFile << "ストリームデータ行 " << i << "\tこれはテスト用のサンプルテキストで、バリエーション "
                << (i % 5) << " があります。" << std::endl;

        // データポイント間の遅延をシミュレート
        if (delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }

        // 進捗を表示
        if (i % 100 == 0) {
            std::cout << "\r" << i << "行を生成しました..." << std::flush;
        }
    }

    outFile.close();
    std::cout << "\r" << lineCount << "行を生成しました。                " << std::endl;
}

int main(int argc, char* argv[]) {
    // 例のためのファイルパスを定義
    std::string tempDir = std::filesystem::temp_directory_path().string();
    std::string streamDataFile = tempDir + "/stream_data.tsv";
    std::string normalizedFile = tempDir + "/normalized_stream.tsv";
    std::string pmiFile = tempDir + "/pmi_stream.tsv";

    try {
        // ステップ1: データストリームをシミュレート
        simulateDataStream(streamDataFile, 1000, 0);

        // ステップ2: stdin/stdoutを使用して処理
        std::cout << "\n例1: 正規化にstdin/stdoutを使用" << std::endl;
        std::cout << "コマンド相当: cat " << streamDataFile << " | suzume-feedmill normalize - -" << std::endl;

        // データをnormalizeを通してパイプするコマンドを作成
        std::string command = "cat " + streamDataFile + " | " +
                             argv[0] + "_normalize_stdin_stdout > " + normalizedFile;

        std::cout << "実行中: " << command << std::endl;

        // 実際の実装では、このコマンドを実行します
        // この例では、直接APIコールでシミュレートします

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 2;

        // "-"はstdin/stdoutを示します
        auto normResult = suzume::normalize(streamDataFile, normalizedFile, normOpt);

        std::cout << "正規化が完了しました！" << std::endl;
        std::cout << normResult.rows << "行を処理し、"
                  << normResult.uniques << "行のユニークな結果を得ました" << std::endl;

        // ステップ3: 処理パイプラインを作成
        std::cout << "\n例2: 処理パイプラインの作成" << std::endl;
        std::cout << "コマンド相当: cat " << streamDataFile
                  << " | suzume-feedmill normalize - - | suzume-feedmill pmi - "
                  << pmiFile << std::endl;

        // 実際の実装では、このパイプラインを実行します
        // この例では、直接APIコールでシミュレートします

        // まず、stdoutに正規化（シミュレーションのために一時ファイルを使用）
        std::string tempNormalized = tempDir + "/temp_normalized.tsv";
        auto pipelineNormResult = suzume::normalize(streamDataFile, tempNormalized, normOpt);

        // 次に、PMIで処理
        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;
        pmiOpt.topK = 500;
        pmiOpt.threads = 2;

        auto pmiResult = suzume::calculatePmi(tempNormalized, pmiFile, pmiOpt);

        std::cout << "パイプライン処理が完了しました！" << std::endl;
        std::cout << "正規化で" << pipelineNormResult.rows << "行、"
                  << "PMI計算で" << pmiResult.grams << "個のn-gramを処理しました" << std::endl;

        // ステップ4: リアルタイム処理シミュレーション
        std::cout << "\n例3: リアルタイム処理シミュレーション" << std::endl;

        // 連続的なデータ生成をシミュレートするバックグラウンドスレッドを開始
        std::string realtimeDataFile = tempDir + "/realtime_data.tsv";
        std::string realtimeOutputFile = tempDir + "/realtime_output.tsv";

        std::cout << "リアルタイムデータ生成を開始..." << std::endl;
        // 実際の実装では、ここでスレッドを開始します
        // この例では、より小さなデータセットを生成します

        simulateDataStream(realtimeDataFile, 500, 0);

        std::cout << "リアルタイムデータを処理中..." << std::endl;

        // 最小限のバッファリングで処理
        suzume::NormalizeOptions realtimeOpt;
        realtimeOpt.form = suzume::NormalizationForm::NFKC;
        realtimeOpt.threads = 1; // リアルタイム処理用に単一スレッド

        auto realtimeResult = suzume::normalize(realtimeDataFile, realtimeOutputFile, realtimeOpt);

        std::cout << "リアルタイム処理が完了しました！" << std::endl;
        std::cout << realtimeResult.rows << "行を処理し、"
                  << realtimeResult.uniques << "行のユニークな結果を得ました" << std::endl;

        // 一時ファイルをクリーンアップ
        std::cout << "\n一時ファイルをクリーンアップ中..." << std::endl;
        std::filesystem::remove(streamDataFile);
        std::filesystem::remove(normalizedFile);
        std::filesystem::remove(pmiFile);
        std::filesystem::remove(tempNormalized);
        std::filesystem::remove(realtimeDataFile);
        std::filesystem::remove(realtimeOutputFile);

        std::cout << "クリーンアップが完了しました！" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
