// suzume-feedmill WebAssemblyモジュールを使用するNode.jsサンプルスクリプト

// WASMモジュールを読み込む
const SuzumeFeedmill = require("./suzume-feedmill.js");

// テスト用サンプルテキスト
const sampleText = `今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～
人工知能の研究が進んでいます。機械学習の応用が広がっています。
自然言語処理技術の発展により、翻訳精度が向上しています。`;

async function runExample() {
  try {
    console.log("suzume-feedmill WASMモジュールを読み込んでいます...");
    const module = await SuzumeFeedmill();
    console.log("モジュールが正常に読み込まれました。\n");

    // ステップ1: テキスト正規化
    console.log("ステップ1: テキスト正規化...");
    const normResult = module.normalize(sampleText, {
      form: "NFKC",
      threads: 2,
    });

    console.log("正規化されたテキスト:");
    console.log(normResult.text);
    console.log(
      `処理された行数: ${normResult.rows}, ユニーク行数: ${normResult.uniques}\n`
    );

    // ステップ2: PMI計算
    console.log("ステップ2: PMI計算...");
    const pmiResult = module.calculatePmi(normResult.text, {
      n: 2,
      topK: 10,
      minFreq: 1,
    });

    console.log("PMI結果:");
    pmiResult.results.forEach((item) => {
      console.log(`${item.ngram}\t${item.score.toFixed(2)}\t${item.frequency}`);
    });
    console.log(`処理されたn-gram数: ${pmiResult.grams}\n`);

    // 単語抽出用にPMI結果をフォーマット
    const pmiText = pmiResult.results
      .map((r) => `${r.ngram}\t${r.score}\t${r.frequency}`)
      .join("\n");

    // ステップ3: 単語抽出
    console.log("ステップ3: 単語抽出...");
    const wordResult = module.extractWords(pmiText, normResult.text, {
      minPmiScore: 1.0,
      minLength: 2,
      maxLength: 10,
      topK: 10,
    });

    console.log("抽出された単語:");
    for (let i = 0; i < wordResult.words.length; i++) {
      console.log(
        `${wordResult.words[i]}\t${wordResult.scores[i].toFixed(2)}\t${
          wordResult.frequencies[i]
        }\t${wordResult.verified[i] ? "✓" : ""}`
      );
    }
    console.log(`抽出された単語数: ${wordResult.words.length}`);

    console.log("\nサンプルが正常に完了しました！");
  } catch (error) {
    console.error("エラー:", error);
  }
}

runExample();
