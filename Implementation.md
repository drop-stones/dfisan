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
  - [x] llvm::Value*をAligned or Unalignedかどうか判定，保存
  - [x] レース関係にあるDefUseを判定, 別に保存
  - [x] 定義のみ(未使用)である命令も保存
  - [x] 定義命令とその定義先を紐付けてProtectInfoに保存
  - [x] 保護データ以外へのUnsafe access命令もProtectInfoに保存
- [x] SafeMallocとの統合
  - [x] SafeMalloc関数をSVFのalloc関数群に登録
  - [ ] safe_mallocで確保されたオブジェクトの型をSVFが認識せず，解析結果が不正確になる問題
    - safe_mallocの置き換え時に，bitcastも計装し，型変換する(?) &larr; もうやってた
- [x] 計装コードとの統合
- [x] 実行時チェックをテスト
- [ ] SPEC2006の保護
  - [x] 401.bzip2, 429.mcf, 433.milc, 456.hmmer, 458.sjeng, 462.libquantum, 445.gobmk
  - [ ] 470.lbm
  - [ ] 482.sphinx3
  - [ ] 464.h264ref
  - [ ] 400.perlbench
  - [ ] 433.gcc
- [ ] 並行バグ検知の実装
  - [x] ~~FSMPTAのalias見逃し &rarr; Andersenで置き換え~~
  - [x] ~~fork,join前後のEdgeが貼られていない~~
    - pthread_create()で渡された関数がread(DfiExtAPI関数)だったせいで，その後の解析もおかしくなっていただけ

### バグ修正

- DefUse解析
  - [x] sscanf()による定義IDがずれる
    - addUnusedDefID()のバグ修正
  - [x] グローバル変数の初期化(struct, pointer)で解析結果にズレ
    - InitialGlobal()内で，構造体の初期化に一つのValue(=定義ID)を割り当て
  - [x] UnusedDefの情報がない
- テストケース
  - [ ] DataRace/data_race.c
  - [x] libc/sscanf
  - [x] NoSupport/ptr_calloc
  - [x] struct_copy
  - [x] libc/memcpy
  - [x] libc/memset
  - [x] struct_init
  - [x] array_init
    - [x] memsetによる一括代入
    - [x] 構造体の配列初期化など，memsetによる各メンバ変数への代入
      - 各メンバ変数への代入を一つのmemset(=定義ID)で扱いたい
    - [x] グローバル構造体の初期化に一つのValue(=定義ID)を割り当てたい
      - getUniqueID()内，各addDefInfo()にて，グローバル変数の初期化は，key + operandをどちらもグローバル変数に設定し，単一のIDを付与
  - [x] global_init
  - [x] local_static_init
  - [x] byval_support
    - 原因: byvalポインタがフツーにaliasと解析されてしまう
    - 解決策:
      - SVFIR(PAG)に細工し，Points-to関係をいじる &larr; 以降の解析にも影響??
      - [x] SVFG上のエッジを削除する (FormalInEdge)

## 外部ライブラリのモデリング + SVFGへの反映

1. DfisanExtAPI.h: 外部ライブラリの定義・使用情報(サマリ)を記述
2. DfisanSVFIRBuilder(元PAGBuilder)::handleExtCall(): 外部ライブラリ呼び出しに対してStore, Load-Edgeを張る
3. DfisanMemSSA::createMUCHI(): Step.2で処理した関数に対する関数呼び出しmu/chiを設定しないよう変更
    - &because; mu/chiは正しく外部ライブラリの処理を反映せず，Step.2のedgeが無効化されてしまうため
    - llvm.memcpyなどもmu/chiを設定せず，サマリを直接用いて処理されている
4. DfisanSVFG::buildSVFG(): 通常通りにmu/chiからSVFGを作成

## グローバル変数の初期化

1. DfisanSVFIRBuilder::InitialGlobal(): 配列初期化に対するStoreEdgeを追加
