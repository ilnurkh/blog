#include <cassert>
#include <deque>
#include <iostream>
#include <functional>
#include <map>
#include <utility>

using TItem = float;
using TItemId = size_t;
using TItemCallback = std::function<void(TItem)>;

class TLongestMonotonicSubseqCollector {
    std::deque<TItem> InputSequence;
    std::deque<TItemId> PrevItems; // if PrevItems[i] == i, it means no prev item exists
    size_t CurrentBiggestSubseqSize = 0;
    TItemId CurrentBiggestSubseqLastItemId = 0;

    using TChainsMap = std::map<TItem, std::pair<TItemId, size_t>>;
    TChainsMap LastChainValueToItemIdAndChainSize;
    std::deque<TChainsMap::iterator> SizeToItemIdToStartFrom;

    void IterateChainStartingByItemId(const TItemCallback& cb, TItemId itemId) const {
        const TItemId& prev = PrevItems.at(itemId);
        if (prev != itemId) {
            IterateChainStartingByItemId(cb, prev);
        }
        cb(InputSequence[itemId]);
    }
public:
    void DumpEach(const TItemCallback& cb) const {
        if (CurrentBiggestSubseqSize == 0) {
            return;
        }
        IterateChainStartingByItemId(cb, CurrentBiggestSubseqLastItemId);
    }

    void Push(TItem newItem) {
        TItemId newItemId = InputSequence.size();
        InputSequence.push_back(newItem);
        TItemId& prevIdOfNewItem = PrevItems.emplace_back(newItemId);

        if (CurrentBiggestSubseqSize == 0) {
            CurrentBiggestSubseqSize = 1;
            CurrentBiggestSubseqLastItemId = newItemId;
            LastChainValueToItemIdAndChainSize[newItem] = std::make_pair(newItemId, 1);
            SizeToItemIdToStartFrom.push_back(LastChainValueToItemIdAndChainSize.begin());
            return;
        }
        TChainsMap::iterator maybeEqualOrLessIter;

        // the new element is the global minimum, so make fast step
        if (LastChainValueToItemIdAndChainSize.begin()->first > newItem) {
            LastChainValueToItemIdAndChainSize.erase(LastChainValueToItemIdAndChainSize.begin());
            auto insertIter = LastChainValueToItemIdAndChainSize.try_emplace(
                newItem, std::make_pair(newItemId, 1));
            assert(insertIter.second);
            SizeToItemIdToStartFrom[0] = insertIter.first;
            return;
        }

        if (LastChainValueToItemIdAndChainSize.size() == 1) {
            maybeEqualOrLessIter = LastChainValueToItemIdAndChainSize.begin();
        } else {
            // this is first element of chains, there the last element is greater than a new one
            TChainsMap::iterator upperBoundIter = LastChainValueToItemIdAndChainSize.upper_bound(newItem);
            // std::cerr << "upperBoundIter=" << upperBoundIter->first << std::endl;
            maybeEqualOrLessIter = upperBoundIter;
            --maybeEqualOrLessIter;
        }
        // std::cerr << "newItem=" << newItem << std::endl;
        // std::cerr << "maybeEqualOrLessIter=" << maybeEqualOrLessIter->first << std::endl;
        if (maybeEqualOrLessIter->first < newItem) {
            // we can continue found maybeEqualOrLessIter chain
            prevIdOfNewItem = maybeEqualOrLessIter->second.first;
            const size_t nonIncreasedChainSize = maybeEqualOrLessIter->second.second;
            // std::cerr << "nonIncreasedChainSize" << nonIncreasedChainSize << std::endl;
            const size_t increasedChainSize = nonIncreasedChainSize + 1;
            if (nonIncreasedChainSize < SizeToItemIdToStartFrom.size()) {
                auto& iterToBestChainOfSameSize = SizeToItemIdToStartFrom[nonIncreasedChainSize];
                if (iterToBestChainOfSameSize->first > newItem) {
                    LastChainValueToItemIdAndChainSize.erase(SizeToItemIdToStartFrom[nonIncreasedChainSize]);
                    auto insertRes = LastChainValueToItemIdAndChainSize.try_emplace(
                        newItem, std::make_pair(newItemId, increasedChainSize)
                    );
                    assert(insertRes.second);
                    iterToBestChainOfSameSize = insertRes.first;
                }
                if (increasedChainSize == SizeToItemIdToStartFrom.size()) {
                    CurrentBiggestSubseqSize = increasedChainSize;
                    CurrentBiggestSubseqLastItemId = newItemId;
                }
            } else {
                auto insertRes = LastChainValueToItemIdAndChainSize.try_emplace(
                    newItem, std::make_pair(newItemId, increasedChainSize)
                );
                assert(insertRes.second);
                assert(nonIncreasedChainSize == SizeToItemIdToStartFrom.size());
                SizeToItemIdToStartFrom.push_back(insertRes.first);
                assert(CurrentBiggestSubseqSize == nonIncreasedChainSize);
                CurrentBiggestSubseqSize = increasedChainSize;
                CurrentBiggestSubseqLastItemId = newItemId;
            }
        } else {
            // in this case - no reason to use current element
            // it's best chain is not the smallest-last chain of all seen chains of the same size
        }
    }
};

int main(int argc, const char* argv[]) {
    TLongestMonotonicSubseqCollector collector;

    while(!std::cin.eof()) {
        TItem item;
        std::cin >> item;
        if (!std::cin.good()) {
            if (!std::cin.eof()) {
                std::cerr << "failed to read item" << std::endl;
                return 1;
            }
            break;
        }
        collector.Push(std::move(item));
    }

    bool first = true;
    collector.DumpEach([&](const TItem& x) {
        if (!first) {
            std::cout << " ";
        }
        first = false;
        std::cout << x;
    });
    std::cout << std::endl;

    return 0;
}
