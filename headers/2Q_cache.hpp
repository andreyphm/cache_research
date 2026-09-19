#pragma once

#include <cstddef>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <optional>
#include <array>

namespace TWO_Q {

enum class Status {
    success,
    not_found,
    already_exists
};

template <typename Data> class Cache {
public:
    static constexpr std::size_t capacity = 8;
    static constexpr std::size_t kin = 2;
    static constexpr std::size_t kout = 4;

    Cache() = default;
    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    [[nodiscard]] Status insert(const std::string& url, Data data) {
        const auto found = map_.find(url);
        if (found == map_.end()) {
            auto& list_a1in = get_list(ListId::A1in_);
            if (list_a1in.size() > kin) {
                auto& list_a1out = get_list(ListId::A1out_);
                evict(list_a1out, std::prev(list_a1out.end()));
                evict_to_ghost();
            } else {
                auto& list_am = get_list(ListId::Am_);
                evict(list_am, std::prev(list_am.end()));
            }

            const auto added = list_a1in.emplace(list_a1in.begin(), url, data);
            const auto result = map_.emplace(added->url_, PageLocation{ListId::A1in_, added});
            if (!result.second) {
                list_a1in.erase(added);
                return Status::already_exists;
            }
            return Status::success;
        }

        auto& list_id = found->second.list_id_;
        auto& list_am = get_list(ListId::Am_);
        switch (list_id) {
            case ListId::A1in_:
                break;

            case ListId::Am_:
                auto& list_am = get_list(ListId::Am_);
                list_am.splice(list_am.begin(), list_am, found->second.iterator_);
                break;

            case ListId::A1out_:
                if (auto& list_a1in = get_list(ListId::A1in_); list_am.size() + list_a1in.size() == capacity && list_a1in.size() > kin) {
                        evict_to_ghost();
                } else {
                    evict(list_am, std::prev(list_am.end()));
                }
                list_id = ListId::Am_;
                break;

            default:
                break;
        }

        found->second.iterator_->data_ = data;
        list_am.splice(list_am.begin(), get_list(list_id), found->second.iterator_);

        return Status::success;
    }

    [[nodiscard]] Status get(const std::string& url, const Data*& data) {
        data = nullptr;

        const auto found = map_.find(url);
        if (found == map_.end()) {
            return Status::not_found;
        }

        const auto page = found->second.iterator_;

        switch (found->second.list_id_) {
            case ListId::A1out_:
                return Status::not_found;

            case ListId::Am_:
                auto& list_am = get_list(ListId::Am_);
                list_am.splice(list_am.begin(), list_am, page);
                break;

            case ListId::A1in_:
            default:
                break;
        }

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

    enum class ListId {
        A1in_,
        A1out_,
        Am_
    };

    struct PageLocation {
        ListId list_id_;
        PageIterator iterator_;
    };

    static constexpr std::size_t number_of_lists_ = 3;

    std::array<PageList, number_of_lists_> lists_;
    std::unordered_map<std::string, PageLocation> map_;

    PageList& get_list(ListId id) {
        return lists_[static_cast<std::size_t>(id)];
    }

    void evict_to_ghost() {
        auto& list_a1in = get_list(ListId::A1in_);
        auto& list_a1out = get_list(ListId::A1out_);
        const auto page = std::prev(list_a1in.end());

        if (list_a1out.size() == kout) {
            evict(std::prev(list_a1out.end()));
        }

        page->data_.reset();
        list_a1out.splice(list_a1out.begin(), list_a1in, page);
        map_.at(page->url_).list_id_ = ListId::A1out_;
    }
    
    void evict(PageList& list, PageIterator page) {
        map_.erase(page->url_);
        list.erase(page);
    }
};

} // namespace TWO_Q
