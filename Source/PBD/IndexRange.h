#pragma once

/*
   連続したインデックス範囲を表す構造体。
   offset : 開始インデックス
   count : 要素数
   例: indices[offset ～ offset + count)
 */
struct IndexRange
{
	int offset = 0;
	int count = 0;

	// 先頭インデックスを返す。
	int begin() const
	{
		return offset;
	}

	// 終端インデックスを返す。 (end は含まない)
	int end() const
	{
		return offset + count;
	}

	// 範囲が空か判定する。
	bool empty() const
	{
		return count == 0;
	}
};