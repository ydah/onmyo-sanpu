# 陰陽算譜

凡陰陽算譜者、以九字記數、憑値於式神、以占斷吉凶、以反閇致往復、遂成算術之法也。

此卷所藏、陰陽算譜之處理系 `onmyo` 也。以椎十一造之。披 `.fu` 之符、析祭文爲字句、結字句爲樹、因其樹而修行法。

## 造作

```sh
make
```

右一度行之、則 `onmyo` 成。

## 修法

```sh
./onmyo examples/fizzbuzz.fu
./onmyo --tokens examples/fizzbuzz.fu
./onmyo --ast examples/fizzbuzz.fu
```

無符者、即修祭文。加 `--tokens` 者、但出字句列、不修。加 `--ast` 者、但出構文樹、不修。

祟品第如左。

- `0`: 成就
- `1`: 修中祟
- `2`: 字句構文亂
- `3`: 入出力引數亂

## 試驗

```sh
./test/run.sh
```

其試驗所驗、標準三祭、託宣、論理、比較、真言相生、糖衣、呼出文、召喚、負歩、限反閇、諸祟、構文亂、并 `--tokens`、`--ast` 起否也。

## 例符

- `examples/fizzbuzz.fu`: 自一至百唱之、占三五徴。
- `examples/kaijo.fu`: 遞修己法、獻階乘。
- `examples/hibo.fu`: 以反閇唱斐波那契列。
