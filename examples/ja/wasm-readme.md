# Suzume Feedmill WebAssemblyモジュール

このパッケージには、文字レベルのn-gramとPMI抽出のための高性能コーパス前処理エンジンであるSuzume FeedmillのWebAssemblyビルドが含まれています。

## 内容

- `suzume-feedmill.js` - JavaScriptグルーコード
- `suzume-feedmill.wasm` - WebAssemblyバイナリ
- `example.html` - ブラウザ用サンプル
- `example.js` - Node.js用サンプル

## ブラウザでの使用方法

```html
<script>
  var Module = {
    onRuntimeInitialized: function() {
      // モジュールの準備完了
      const result = Module.normalize("テキストを入力", {
        form: 'NFKC',
        threads: 2
      });
      console.log(result);
    }
  };
</script>
<script src="suzume-feedmill.js"></script>
```

## Node.jsでの使用方法

```javascript
const SuzumeFeedmill = require('./suzume-feedmill.js');

SuzumeFeedmill().then(module => {
  const result = module.normalize("テキストを入力", {
    form: 'NFKC',
    threads: 2
  });
  console.log(result);
});
```

## API

### normalize(text, options)

テキストを正規化し、重複を削除します。

オプション:
- `form`: 正規化形式（'NFKC'または'NFC'、デフォルト: 'NFKC'）
- `threads`: スレッド数（デフォルト: 2）

### calculatePmi(text, options)

文字n-gramのPMI（相互情報量）を計算します。

オプション:
- `n`: N-gramサイズ（1, 2, または 3、デフォルト: 2）
- `topK`: 返す上位結果の数（デフォルト: 2500）
- `minFreq`: 最小頻度閾値（デフォルト: 3）
- `threads`: スレッド数（デフォルト: 2）

### extractWords(pmiText, originalText, options)

PMI結果から潜在的な未知語を抽出します。

オプション:
- `minPmiScore`: 最小PMIスコア閾値（デフォルト: 3.0）
- `minLength`: 最小単語長（デフォルト: 2）
- `maxLength`: 最大単語長（デフォルト: 10）
- `topK`: 返す上位結果の数（デフォルト: 100）
- `threads`: スレッド数（デフォルト: 2）

## サンプル

完全なサンプルについては、`example.html`と`example.js`を参照してください。
