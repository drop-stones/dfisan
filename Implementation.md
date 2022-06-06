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

## No Support

- [ ] Static variable
  - 理由: Static変数をまとめて解析してしまう &rarr; 保護精度 & 実行時コストに悪影響

## TODO

- [ ] `const global`ノードへのチェック関数を削除 (最適化)
- [ ] チェック関数の重複を削除 (最適化)
- [ ] 小規模リアル検体でテスト