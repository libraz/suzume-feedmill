#include <suzume_feedmill.h>
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * 統計情報JSON出力の例
 *
 * この例では以下を示します:
 * 1. --stats-jsonオプションをプログラムで使用する方法
 * 2. JSON統計情報の解析と分析
 * 3. パフォーマンスメトリクスのモニタリング
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
    std::filesystem::create_directories(outputDir);

    // 出力ファイルを定義
    std::string normalizedFile = outputDir + "/normalized.tsv";
    std::string pmiFile = outputDir + "/ngrams.tsv";
    std::string statsFile = outputDir + "/stats.json";

    try {
        // ステップ1: テキストを正規化して統計情報を収集
        std::cout << "テキストを正規化して統計情報を収集中..." << std::endl;

        suzume::NormalizeOptions normOpt;
        normOpt.form = suzume::NormalizationForm::NFKC;
        normOpt.threads = 4;
        // 出力を混乱させないためにプログレスコールバックは使用しない

        auto normResult = suzume::normalize(inputFile, normalizedFile, normOpt);

        // 正規化統計情報のJSONオブジェクトを作成
        json normStats = {
            {"command", "normalize"},
            {"input", inputFile},
            {"output", normalizedFile},
            {"rows", normResult.rows},
            {"uniques", normResult.uniques},
            {"duplicates", normResult.duplicates},
            {"elapsed_ms", normResult.elapsedMs},
            {"mb_per_sec", normResult.mbPerSec}
        };

        std::cout << "正規化統計情報:" << std::endl;
        std::cout << normStats.dump(2) << std::endl;

        // ステップ2: PMIを計算して統計情報を収集
        std::cout << "\nPMIを計算して統計情報を収集中..." << std::endl;

        suzume::PmiOptions pmiOpt;
        pmiOpt.n = 2;
        pmiOpt.topK = 1000;
        pmiOpt.minFreq = 3;
        pmiOpt.threads = 4;
        // 出力を混乱させないためにプログレスコールバックは使用しない

        auto pmiResult = suzume::calculatePmi(normalizedFile, pmiFile, pmiOpt);

        // PMI統計情報のJSONオブジェクトを作成
        json pmiStats = {
            {"command", "pmi"},
            {"input", normalizedFile},
            {"output", pmiFile},
            {"n", pmiOpt.n},
            {"grams", pmiResult.grams},
            {"distinct_ngrams", pmiResult.distinctNgrams},
            {"elapsed_ms", pmiResult.elapsedMs},
            {"mb_per_sec", pmiResult.mbPerSec}
        };

        std::cout << "PMI統計情報:" << std::endl;
        std::cout << pmiStats.dump(2) << std::endl;

        // ステップ3: 統計情報を組み合わせてファイルに保存
        json allStats = {
            {"normalize", normStats},
            {"pmi", pmiStats},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()},
            {"total_processing_time_ms", normResult.elapsedMs + pmiResult.elapsedMs}
        };

        // 組み合わせた統計情報をファイルに書き込み
        std::ofstream statsOut(statsFile);
        if (!statsOut.is_open()) {
            throw std::runtime_error("統計ファイルを書き込み用に開けませんでした: " + statsFile);
        }
        statsOut << allStats.dump(2) << std::endl;
        statsOut.close();

        // ステップ4: 統計情報を分析
        std::cout << "\n統計情報を分析中..." << std::endl;

        // 重複率を計算
        double duplicateRate = static_cast<double>(normResult.duplicates) / normResult.rows * 100.0;
        std::cout << "重複率: " << std::fixed << std::setprecision(2) << duplicateRate << "%" << std::endl;

        // 平均処理速度を計算
        double avgSpeed = (normResult.mbPerSec + pmiResult.mbPerSec) / 2.0;
        std::cout << "平均処理速度: " << std::fixed << std::setprecision(2) << avgSpeed << " MB/秒" << std::endl;

        // n-gram密度を計算
        double ngramDensity = static_cast<double>(pmiResult.distinctNgrams) / normResult.uniques;
        std::cout << "N-gram密度: " << std::fixed << std::setprecision(2) << ngramDensity
                  << " ユニーク行あたりの異なるn-gram" << std::endl;

        std::cout << "\n結果は以下に保存されました:" << std::endl;
        std::cout << "  - 正規化テキスト: " << normalizedFile << std::endl;
        std::cout << "  - PMI結果: " << pmiFile << std::endl;
        std::cout << "  - 統計情報: " << statsFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "エラー: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
