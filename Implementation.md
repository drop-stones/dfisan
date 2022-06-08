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
- [x] bitcast Valueへの対応
  - 理由: `memcpy(i8* (bitcast %struct* to i8*), ...)`
  - 解決: `llvm::Value::stripPointerCasts()`: bitcastを外した値を返す

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