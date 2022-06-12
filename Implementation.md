# Implementation details

## Support

- [x] Local variable
- [x] Global primitive variable
- [x] Global struct with initialization
- [x] Global array of primitive type
- [x] Global struct with zeroinitializer
  - `struct S s;`
  - 解決: SVFIRBuilder内で，zeroinitializerノードをPAGに追加
- [x] Global array of struct type
  - `struct S arr[8] = {...};`
  - 解決: SVFG構築後，グローバル変数の初期化・memcpy()によるローカル変数の初期化を表すStoreSVFGNodeをマージ

## Bug fixes

- [x] Global initialization with not const values
  - 理由: 実装によるバグ (at DataFlowIntegritySanitizer.cpp)
    ```
    %160 = getelementptr inbounds [7 x i32], %10, 0, 0
    %161 = bitcast i32* to i8* %160
    call llvm.memcpy(%159, %161)
    ```
- [x] bitcast Valueへの対応
  - 理由: `memcpy(i8* (bitcast %struct* to i8*), ...)`
  - 解決: `llvm::Value::stripPointerCasts()`: bitcastを外した値を返す
- [x] 配列のコピー
  - 理由: `memcpy()`の引数に関する`bitcast`に対して未対応
- [x] Paddingによる構造体のmemcpy()における誤検知
  - 理由: `memcpy()`はField-sensitiveに定義IDを割り振るが，padding配列にたいしても定義IDを割り振ってしまい，他メンバ変数の定義IDを上書きしてしまう
  - 解決: `PaddingFieldSet`にpaddingである変数を保存させ，判定する
- [x] ポインタによるmemcpy()
  - 理由: `memcpy()`の引数がポインタ変数であり，型・サイズが推測できない場合に未対応
  - 解決: `getBaseType()`に修正

## TODO

### 必須事項

- [ ] Static variable
  - 理由: Static変数をまとめて解析してしまう &rarr; 保護精度 & 実行時コストに悪影響
  - 解決案: UseDef解析時，UseDefを結ぶ際にエイリアス関係にあるかどうかをチェック
- [ ] 小規模リアル検体でテスト

### 最適化

- [ ] `const global`ノードへのチェック関数を削除
- [ ] チェック関数の重複を削除
- [ ] チェック関数の高速化