# suzume-feedmill

> Grind the feed, sharpen the tokens.

文字レベルの n-gram と PMI 抽出のための高性能コーパス前処理エンジンで、Suzume シリーズ（SuzumeFeed など）に「未知語候補の粉」を供給します。

## 特徴

- **高性能テキスト正規化**

  - Unicode 正規化（NFKC/NFC）
  - Bloom フィルタによる重複除去
  - マルチスレッド処理

- **高速文字レベル n-gram と PMI 抽出**

  - 1〜3 文字の n-gram をサポート
  - min-heap を使用した上位 K 件の PMI 抽出
  - 設定可能な最小頻度閾値

- **未知語抽出**（プロジェクトの主目的のためのコア機能）

  - 高 PMI スコアの n-gram からの候補生成
  - 元テキストでの検証
  - 統計的・文脈的分析に基づくフィルタリング
  - 潜在的な未知語のランキング
  - 真の未知語を発見するためのコンポーネントベースのパイプライン

- **クロスプラットフォーム**

  - Linux（glibc/musl）、macOS（Intel/ARM）、Windows（x64）用のプリビルドバイナリ
  - スタンドアロン CLI として利用可能
  - WebAssembly対応でブラウザやNode.js環境でも利用可能

- **文字レベル処理**
  - 明示的な単語境界のない言語（日本語、中国語など）向けに設計
  - 既存のトークナイザーでは認識されない真の未知語を発見
  - 意味のある可能性のある部分語パターンを識別

## ユースケース

- **NLP 辞書構築**: 大規模テキストコーパスを処理して、辞書や言語モデル用の潜在的な新語や連語を抽出
- **未知語発見**: 新しい用語や概念を表す可能性のある統計的に有意な文字の組み合わせを特定
- **テキストマイニング**: ソーシャルメディア投稿、ニュース記事、科学論文などの大規模データセットから意味のあるパターンを抽出
- **言語学習教材**: 言語学習アプリケーション向けの頻度リストと一般的な文字の組み合わせを生成
- **検索エンジン最適化**: コンテンツ最適化のための重要なフレーズと関連用語を特定

## インストール

### ソースからのビルド

```bash
# リポジトリをクローン
git clone https://github.com/yourusername/suzume-feedmill.git
cd suzume-feedmill

# ビルドディレクトリを作成
mkdir build && cd build

# CMakeで設定
cmake ..

# ビルド
make

# インストール（オプション）
sudo make install
```

### プリビルドバイナリの使用

様々なプラットフォーム用のプリビルドバイナリは[リリースページ](https://github.com/yourusername/suzume-feedmill/releases)で入手できます。

## クイックスタートガイド

suzume-feedmill を使い始めるための簡単な手順を紹介します：

```bash
# テキストファイルを正規化（重複を除去し、テキストを標準化）
suzume-feedmill normalize input.tsv normalized.tsv

# 文字 n-gram の PMI を計算
suzume-feedmill pmi normalized.tsv ngrams.tsv --n 2 --top 5000

# 未知語を抽出
suzume-feedmill word-extract ngrams.tsv normalized.tsv words.tsv
```

入力と出力の形式の詳細な例については、下記の「データ形式」セクションを参照してください。

## CLI 使用法

### テキスト正規化

```bash
suzume-feedmill normalize <in.tsv> <out.tsv> [オプション]

オプション:
  --threads N         スレッド数（デフォルト: 論理コア数）
  --form NFKC|NFC     正規化形式（デフォルト: NFKC）
  --bloom-fp 0.01     Bloomフィルタの偽陽性率
  --progress tty|json|none  進捗報告形式（デフォルト: tty）
```

### PMI 計算

```bash
suzume-feedmill pmi <in.txt> <out.tsv> [オプション]

オプション:
  --n 1|2|3           n-gramサイズ（デフォルト: 2）
  --top K             上位結果数（デフォルト: 2500）
  --min-freq N        最小頻度閾値（デフォルト: 3）
  --threads N         スレッド数（デフォルト: 論理コア数）
  --progress tty|json|none  進捗報告形式（デフォルト: tty）
```

### 未知語抽出

```bash
suzume-feedmill word-extract <pmi-results.tsv> <original-text.txt> <output.tsv> [オプション]

オプション:
  --min-score N       最小PMIスコア閾値（デフォルト: 3.0）
  --min-length N      最小単語長（デフォルト: 2）
  --max-length N      最大単語長（デフォルト: 10）
  --top K             上位結果数（デフォルト: 100）
  --verify true|false 元テキストで候補を検証（デフォルト: true）
  --threads N         スレッド数（デフォルト: 論理コア数）
  --progress tty|json|none  進捗報告形式（デフォルト: tty）
```

## C++ API

ライブラリは独自のアプリケーションに統合するためのC++ APIを提供しています：

```cpp
#include <suzume_feedmill.h>
#include <iostream>

int main() {
  // テキスト正規化
  suzume::NormalizeOptions normOpt;
  normOpt.threads = 8;
  auto normResult = suzume::normalize("raw.tsv", "clean.tsv", normOpt);

  std::cout << "処理行数: " << normResult.rows << "行, "
            << "ユニーク行数: " << normResult.uniques << "行, "
            << "処理時間: " << normResult.elapsedMs << "ms" << std::endl;

  // PMI計算
  suzume::PmiOptions pmiOpt;
  pmiOpt.n = 2;
  pmiOpt.topK = 2500;
  auto pmiResult = suzume::calculatePmi("clean.tsv", "grams.tsv", pmiOpt);

  std::cout << "処理n-gram数: " << pmiResult.grams << ", "
            << "処理時間: " << pmiResult.elapsedMs << "ms" << std::endl;

  return 0;
}
```

## データ形式

suzume-feedmill は入出力ファイルに特定のデータ形式を使用します：

### 入力テキスト形式

入力テキストファイルは 1 行に 1 つのテキストエントリを含む必要があります。これは文、段落、または処理したいテキスト単位です：

```
今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～
人工知能の研究が進んでいます。機械学習の応用が広がっています。
自然言語処理技術の発展により、翻訳精度が向上しています。
```

### 正規化テキスト形式

正規化テキストファイルには、重複が除去され、正規化されたテキストエントリが含まれます。正規化処理では、以下の変換が適用されます：

- 重複エントリの除去
- 空白の標準化
- 句読点の正規化
- Unicode正規化（NFKC/NFC）の適用

```
今日は良い天気ですね。明日も晴れるといいな。
人工知能の研究が進んでいます。機械学習の応用が広がっています。
自然言語処理技術の発展により、翻訳精度が向上しています。
```

注：この例では、入力の最初の3行は正規化後に重複と見なされ、1行にまとめられています。

### N-gram PMI 形式

n-gram PMI ファイルは 3 つの列を持つタブ区切り値（TSV）ファイルです：

1. n-gram: 文字の並び
2. PMI スコア: 相互情報量スコア
3. 頻度: この n-gram がコーパスに出現する回数

```
人工	8.76	127
工知	7.92	98
知能	7.45	76
```

### 単語抽出形式

単語抽出の出力は 4 つの列を持つタブ区切り値（TSV）ファイルです：

1. 単語: 抽出された単語候補
2. スコア: 平均 PMI スコア
3. 構成要素: 構成 n-gram の数
4. 検証: 元のテキストに存在する場合はチェックマーク（✓）

```
人工知能	7.38	3	✓
機械学習	6.94	2	✓
自然言語処理	5.87	4	✓
```

## 実際のワークフロー例

大規模コーパスから未知語を発見するための典型的なワークフローを紹介します：

1. **データ収集**: 大量のテキストデータを収集（Web ページ、記事、SNS 投稿など）

   ```bash
   # 例: 100万ツイートをtweets.tsvに収集
   ```

2. **テキスト正規化**: テキストデータのクリーニングと正規化

   ```bash
   suzume-feedmill normalize tweets.tsv normalized.tsv --threads 8
   ```

   入力例（tweets.tsv）- 1 行に 1 つのテキストエントリ:

   ```
   今日は良い天気ですね。明日も晴れるといいな。
   今日は　良い天気ですね。明日も晴れるといいな。
   今日は良い天気ですね！明日も晴れるといいな～
   人工知能の研究が進んでいます。機械学習の応用が広がっています。
   自然言語処理技術の発展により、翻訳精度が向上しています。
   ```

   出力例（normalized.tsv）- 正規化され、重複が除去されたテキスト:

   ```
   今日は良い天気ですね。明日も晴れるといいな。
   人工知能の研究が進んでいます。機械学習の応用が広がっています。
   自然言語処理技術の発展により、翻訳精度が向上しています。
   ```

   （注: 重複やバリエーションが除去され、空白が正規化され、句読点が標準化されます）

3. **PMI 計算**: 統計的に有意な文字レベルの n-gram を抽出

   ```bash
   suzume-feedmill pmi normalized.tsv ngrams.tsv --n 2 --top 5000 --min-freq 5
   ```

   出力例（ngrams.tsv）- n-gram、スコア、頻度のタブ区切り値:

   ```
   人工	8.76	127
   工知	7.92	98
   知能	7.45	76
   機械	7.21	64
   学習	6.89	112
   自然	6.54	89
   然言	6.32	85
   言語	6.21	92
   語処	5.98	78
   処理	5.87	81
   ```

   （形式: n-gram[TAB]PMI スコア[TAB]頻度）

4. **未知語抽出**: PMI 結果から潜在的な未知語を抽出

   ```bash
   suzume-feedmill word-extract ngrams.tsv normalized.tsv potential-words.tsv --min-score 3.0 --top 100
   ```

   出力例（potential-words.tsv）- 抽出された単語とメタデータのタブ区切り値:

   ```
   人工知能	7.38	3	✓
   機械学習	6.94	2	✓
   自然言語処理	5.87	4	✓
   深層学習	5.21	2	✓
   ニューラルネットワーク	4.95	5	✓
   教師あり学習	4.76	3	✓
   データマイニング	4.52	3	✓
   ```

   （形式: 単語[TAB]平均 PMI スコア[TAB]構成 n-gram 数[TAB]検証ステータス）

   検証ステータス（✓）は、その単語が元のテキストに存在したことを示します。元のテキストには出現しないが、高 PMI の n-gram から構築された単語にはこのマークがつきません。

## アーキテクチャ

- **コア**: SIMD 最適化を含む C++17 実装
- **正規化**: UTF-8 検証と正規化のための ICU
- **重複除去**: 効率的な重複検出のための xxHash64 + BloomFilter
- **データ構造**: ロックフリースレッドマージングを備えた robin_hood ハッシュマップ
- **PMI 計算**: 上位 K 抽出のためのメモリマップ頻度カウントと min-heap
- **未知語抽出**: 候補生成、検証、フィルタリング、ランキングのためのコンポーネントベースのパイプライン

## プロジェクト構造

```
/src            C++コア実装
  /cli          CLI実装
  /core         コア機能実装
  /io           入出力処理
  /parallel     並列処理
  /third_party  サードパーティライブラリ
  /wasm         WebAssemblyバインディング
/include        公開ヘッダ
/bin            実行可能ファイル
/examples       使用例
/tests          テスト
/CMakeLists.txt ビルド設定
```

## ソースからのビルド

```bash
# ビルドディレクトリを作成
mkdir build && cd build

# CMakeで設定
cmake ..

# ビルド
make

# テストを実行
make test
```

## WebAssembly対応

suzume-feedmillはWebAssembly（WASM）にコンパイルして、WebブラウザやNode.js環境で使用することができます。これにより、サーバーサイド処理なしでブラウザ上で直接ライブラリを実行できます。

### WebAssembly向けのビルド

WebAssemblyモジュールをビルドするには、まずEmscriptenをインストールする必要があります：

```bash
# Emscriptenのインストール（まだインストールされていない場合）
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

次に、WebAssemblyモジュールをビルドします：

```bash
# ビルドディレクトリを作成
mkdir build-wasm && cd build-wasm

# Emscriptenで設定
emcmake cmake .. -DBUILD_WASM=ON

# ビルド
emmake make
```

これにより、以下のファイルが生成されます：

- `suzume-feedmill.js`: JavaScriptグルーコード
- `suzume-feedmill.wasm`: WebAssemblyバイナリ

### Webブラウザでの使用例

```html
<!DOCTYPE html>
<html>
  <head>
    <title>suzume-feedmill WASMデモ</title>
  </head>
  <body>
    <h1>suzume-feedmill WASMデモ</h1>
    <textarea id="input" rows="10" cols="50">
今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～</textarea
    >
    <button id="normalize">正規化</button>
    <div id="output"></div>

    <script>
      // WASMモジュールを読み込む
      var Module = {
        onRuntimeInitialized: function () {
          document
            .getElementById("normalize")
            .addEventListener("click", function () {
              var input = document.getElementById("input").value;

              // normalize関数を呼び出す
              var result = Module.normalize(input, {
                form: "NFKC",
                threads: 2,
              });

              // 結果を表示
              document.getElementById("output").innerHTML =
                "<h3>正規化されたテキスト:</h3>" +
                "<pre>" +
                result.text +
                "</pre>" +
                "<h3>統計:</h3>" +
                "<p>行数: " +
                result.rows +
                "</p>" +
                "<p>ユニーク行数: " +
                result.uniques +
                "</p>" +
                "<p>削除された重複: " +
                result.duplicates +
                "</p>";
            });
        },
      };
    </script>
    <script src="suzume-feedmill.js"></script>
  </body>
</html>
```

### Node.jsでの使用例

```javascript
// WASMモジュールを読み込む
const SuzumeFeedmill = require("./suzume-feedmill.js");

SuzumeFeedmill().then((module) => {
  // テキストを正規化
  const text = `今日は良い天気ですね。明日も晴れるといいな。
今日は　良い天気ですね。明日も晴れるといいな。
今日は良い天気ですね！明日も晴れるといいな～`;

  const normResult = module.normalize(text, {
    form: "NFKC",
    threads: 2,
  });

  console.log("正規化されたテキスト:");
  console.log(normResult.text);
  console.log(
    `${normResult.rows}行を処理し、${normResult.uniques}行のユニークな結果を得ました`,
  );

  // PMIを計算
  const pmiResult = module.calculatePmi(normResult.text, {
    n: 2,
    topK: 10,
    minFreq: 1,
  });

  console.log("\nPMI結果:");
  pmiResult.results.forEach((item) => {
    console.log(`${item.ngram}\t${item.score.toFixed(2)}\t${item.frequency}`);
  });
});
```

## ライセンス

MIT

## 技術用語解説

- **Bloom フィルタ**: 要素がセットのメンバーであるかどうかをテストするための空間効率の良い確率的データ構造。suzume-feedmill では、最小限のメモリ使用量で効率的な重複検出に使用されています。

- **PMI (Pointwise Mutual Information、相互情報量)**: 2 つの事象間（この場合は文字の共起）の関連性を示す統計的指標。高い PMI スコアは、偶然よりも頻繁に一緒に出現する文字を示します。

- **n-gram**: テキストサンプルから連続する n 個の項目（この場合は文字）の並び。suzume-feedmill は文字レベルの n-gram（1〜3 文字）を扱います。

- **min-heap（最小ヒープ）**: 親ノードが常にその子ノード以下である二分木データ構造。PMI 計算における効率的な上位 K 件抽出に使用されます。

- **ICU (International Components for Unicode)**: Unicode サポートとソフトウェア国際化のための成熟した広く使用されている C/C++ ライブラリセット。UTF-8 検証と正規化に使用されています。

- **xxHash64**: 高速な非暗号化ハッシュアルゴリズムで、重複検出プロセスでの効率的な文字列ハッシュ化に使用されています。Bloom フィルタとハッシュマップの両方に初期ハッシュ値を提供します。

- **robin_hood ハッシュマップ**: プローブシーケンス長のばらつきを減らすためにロビンフッドハッシュを使用するオープンアドレッシングハッシュテーブル実装。効率的なメモリ内データ構造に使用されています。

これらの技術は重複検出プロセスで連携して動作します：xxHash64 が文字列を高速にハッシュ化し、Bloom フィルタが高速な初期チェック（偽陽性の可能性あり）を提供し、robin_hood ハッシュマップが必要に応じて正確な検証を行います。この二段階アプローチにより、速度と精度を兼ね備えています。

## 文字レベル処理に関する重要な注意

suzume-feedmill は**文字レベル**で動作し、単語レベルではありません。これは設計上の特徴であり、特に以下の用途に有用です：

1. **明示的な単語境界のない言語**（日本語、中国語など）
2. **既存のトークナイザーでは認識されない真の未知語の発見**
3. **意味のある可能性のある部分語パターンの識別**

単語レベルの分析を行うには、通常以下の手順を踏みます：

1. suzume-feedmill を使用して興味深い文字の組み合わせを特定
2. 後処理を適用して隣接する高 PMI の n-gram を組み合わせる
3. 頻度、長さ、その他の基準に基づいて結果をフィルタリング

`examples/`ディレクトリのサンプルは、基本的な使用方法とより高度な後処理技術の両方を示しています。

## 関連プロジェクト

- SuzumeFeed（近日公開予定） - suzume-feedmill の出力を使用してトークン化モデルを強化
