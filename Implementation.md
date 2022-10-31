# 実装進捗

- [x] DfisanSVFGBuilderによるSVFG作成への介入
- [x] DfisanSVFGによる，直接のSVFG作成への介入
- [x] DfisanMemSSAによる，MemSSA作成への介入
  - ただ，MemSSAの段階でmu/chiの付与をExternal APIに対して行えない
  - &because; mu/chiは元々LoadStmt/StoreStmtにのみ付与できるもの
- [x] DfisanSVFIRBuilderによる，外部ライブラリ呼び出しの処理への介入
  - StoreStmt, LoadStmtを付与できる
- [x] SVFGの余分なNode, Edgeの削除
  - StrongUpdateであるEdgeの削除
  - DirectEdgeの削除 (&because; top-level変数のdef-useは不要なため)

## TODO

- [ ] ExtAPIへの対応
  - [x] calloc
  - [x] read
  - [x] sscanf
- [x] Global変数の初期化
  - [x] Array
- [x] byval pointerへの対応
  - byvalポインタの使用に対応する定義がないが，保護対象と指定されなければ問題なし()
- [ ] DefUse解析
  - [x] DefUseIDMap(NodeIDの対応)を計算
    - DefUseSolver::solve()
  - [x] NodeIDのマージ(e.g., Field毎のmemcpyに一つのID)
    - DefUseSolver::getUniqueDefID()
  - [x] Renaming最適化
    - DefUseSolver::calcEquivalentDefSet() + registerDefUse()
  - [x] NodeIDからllvm::Value*への変換
    - DefUseSolver::registerDefUse()
  - [ ] llvm::Value*をAligned or Unalignedかどうか判定，保存
  - [x] レース関係にあるDefUseを判定, 別に保存
  - [ ] 定義のみ(未使用)である命令も保存
- [ ] SafeMallocとの統合
  - [ ] SafeMalloc関数をSVFのalloc関数群に登録
- [ ] 計装コードとの統合
- [ ] 実行時チェックをテスト
- [ ] SPEC2006の保護
- [ ] 並行バグ検知の実装

## 外部ライブラリのモデリング + SVFGへの反映

1. DfisanExtAPI.h: 外部ライブラリの定義・使用情報(サマリ)を記述
2. DfisanSVFIRBuilder(元PAGBuilder)::handleExtCall(): 外部ライブラリ呼び出しに対してStore, Load-Edgeを張る
3. DfisanMemSSA::createMUCHI(): Step.2で処理した関数に対する関数呼び出しmu/chiを設定しないよう変更
    - &because; mu/chiは正しく外部ライブラリの処理を反映せず，Step.2のedgeが無効化されてしまうため
    - llvm.memcpyなどもmu/chiを設定せず，サマリを直接用いて処理されている
4. DfisanSVFG::buildSVFG(): 通常通りにmu/chiからSVFGを作成

## グローバル変数の初期化

1. DfisanSVFIRBuilder::InitialGlobal(): 配列初期化に対するStoreEdgeを追加
