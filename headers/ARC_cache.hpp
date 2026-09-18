#pragma once

#include <cstddef>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <optional>
#include <array>

namespace ARC {

enum class Status {
    success,
    not_found,
    already_exists
};

enum class ListId {
    T1,
    T2,
    B1,
    B2
};

template <typename Data> class Cache {
public:
    static constexpr std::size_t capacity = 4;
    static constexpr std::size_t number_of_lists = 4;
    std::size_t size_parameter = 2;

    Cache() = default;
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    [[nodiscard]] Status insert(const std::string& url, Data data) {
        const auto found = map_.find(url);
        if (found == map_.end()) {
            auto& list_t1 = get_list(ListId::T1);
            auto& list_b1 = get_list(ListId::B1);

            if (list_t1.size() + list_b1.size() == capacity) {
                if (list_t1.size() == capacity) {
                    evict(list_t1, std::prev(list_t1.end()));
                } else {
                    evict(list_b1, std::prev(list_b1.end()));
                    evict_to_ghost(false);
                }
            } else {
                auto& list_b2 = get_list(ListId::B2);
                const auto total_size = list_t1.size() + get_list(ListId::T2).size() + list_b1.size() + list_b2.size();

                if (total_size >= capacity) {
                    if (total_size == 2 * capacity) {
                        evict(list_b2, std::prev(list_b2.end()));
                    }
                    evict_to_ghost(false);
                }
            }

            const auto added = list_t1.emplace(list_t1.begin(), url, data);
            const auto result = map_.emplace(added->url_, PageLocation{ListId::T1, added});
            if (!result.second) {
                list_t1.erase(added);
                return Status::already_exists;
            }
            return Status::success;
        }

        auto& list_id = found->second.list_id;
        switch(list_id) {
            case ListId::B1:
                size_parameter++;
                evict_to_ghost(false);
                found->second.iterator->data_ = data;
                break;

            case ListId::B2:
                size_parameter--;
                evict_to_ghost(true);
                found->second.iterator->data_ = data;
                break;

            default:
                break;
        }

        auto& list_t2 = get_list(ListId::T2);
        list_t2.splice(list_t2.begin(), get_list(list_id), found->second.iterator);
        list_id = ListId::T2;

        return Status::success;
    }

    [[nodiscard]] Status get(const std::string& url, const Data*& data) {
        data = nullptr;

        const auto found = map_.find(url);
        if (found == map_.end()) {
            return Status::not_found;
        }

        auto& source_list_id = found->second.list_id;
        if (source_list_id == ListId::B1 || source_list_id == ListId::B2) {
            return Status::not_found;
        }

        auto& list_t2 = get_list(ListId::T2);
        auto& source_list = get_list(source_list_id);
        const auto page = found->second.iterator;
        list_t2.splice(list_t2.begin(), source_list, page);
        source_list_id = ListId::T2;

        data = std::addressof(*page->data_);

        return Status::success;
    }

private:
    struct Page {
        Page(const std::string& url, Data data): url_(url), data_(data) {}

        std::string url_;
        std::optional<Data> data_;
    };

    using PageList = std::list<Page>;
    using PageIterator = PageList::iterator;

    struct PageLocation {
        ListId list_id;
        PageIterator iterator;
    };

    std::array<PageList, number_of_lists> lists_;
    std::unordered_map<std::string, PageLocation> map_;

    PageList& get_list(ListId id) {
        return lists_[static_cast<std::size_t>(id)];
    }

    void evict_to_ghost(bool is_b2_hit) {
        auto& list_t1 = get_list(ListId::T1);

        const bool evict_from_t1 = !list_t1.empty() && (list_t1.size() > size_parameter
                                   || (is_b2_hit && list_t1.size() == size_parameter));

        const auto source_id = evict_from_t1 ? ListId::T1 : ListId::T2;
        const auto ghost_id = evict_from_t1 ? ListId::B1 : ListId::B2;

        auto& source = get_list(source_id);
        auto& ghost = get_list(ghost_id);
        const auto page = std::prev(source.end());

        page->data_.reset();
        ghost.splice(ghost.begin(), source, page);
        map_.at(page->url_).list_id = ghost_id;
    }

    void evict(PageList& list, PageIterator page) {
        map_.erase(page->url_);
        list.erase(page);
    }
};

} // namespace ARC
