﻿/*🍲Ketl🍲*/
#ifndef bnf_nodes_h
#define bnf_nodes_h

#include "parser.h"

namespace Ketl {

	class BnfNodeLiteral : public BnfNode {
	public:
		virtual ~BnfNodeLiteral() = default;

		BnfNodeLiteral(const std::string_view& value, bool utility = false)
			: _value(value), _utility(utility) {}

		std::pair<bool, std::unique_ptr<Node>> parse(BnfIterator& it) const;

		bool process(BnfIterator& it, ProcessNode& parentProcess) const override;

		const std::string_view& value() const {
			return _value;
		}

		std::string_view errorMsg() const override {
			return value();
		}

	private:
		std::string_view _value;
		bool _utility;
	};

	class BnfNodeId : public BnfNode {
	public:

		BnfNodeId(const std::string_view& id, bool weakContent = false)
			: _id(id), _weakContent(weakContent) {}

		void preprocess(const BnfManager& manager) override;

		bool process(BnfIterator& it, ProcessNode& parentProcess) const override;

	private:

		const BnfNode* _node = nullptr;
		std::string _id;
		bool _weakContent;
	};

	class BnfNodeLeaf : public BnfNode {
	public:

		enum class Type : uint8_t {
			Id,
			Number,
			String,
		};

		BnfNodeLeaf(Type type)
			: _type(type) {}

		std::pair<bool, std::unique_ptr<Node>> parse(BnfIterator& it) const;

		const Type& type() const {
			return _type;
		}

		bool process(BnfIterator& it, ProcessNode& parentProcess) const override;

		std::string_view errorMsg() const override {
			switch (type()) {
			case Type::Id: {
				static const std::string id = "id";
				return id;
			}
			case Type::Number: {
				static const std::string number = "number";
				return number;
			}
			case Type::String: {
				static const std::string str = "string literal";
				return str;
			}
			}
			return {};
		}

	private:
		Type _type;
	};

	class BnfNodeConcat : public BnfNode {
	public:

		template<class... Args>
		BnfNodeConcat(Args&&... args) {
			std::unique_ptr<BnfNode> nodes[] = { std::move(args)... };
			_firstChild = std::move(nodes[0]);
			auto* it = &_firstChild;
			for (auto i = 1u; i < sizeof...(Args); ++i) {
				it->get()->nextSibling = std::move(nodes[i]);
				it = &it->get()->nextSibling;
			}
		}

		void preprocess(const BnfManager& manager) override {
			_firstChild->preprocess(manager);
			BnfNode::preprocess(manager);
		}

		bool process(BnfIterator& it, ProcessNode& parentProcess) const override;

	private:

		std::unique_ptr<BnfNode> _firstChild;
	};

	class BnfNodeOr : public BnfNode {
	public:

		template<class... Args>
		BnfNodeOr(Args&&... args) {
			_children.reserve(sizeof...(Args));
			std::unique_ptr<BnfNode> nodes[] = { std::move(args)... };
			for (auto& node : nodes) {
				_children.emplace_back(std::move(node));
			}
		}

		void preprocess(const BnfManager& manager) override {
			for (auto& child : _children) {
				child->preprocess(manager);
			}
			BnfNode::preprocess(manager);
		}

		bool process(BnfIterator& it, ProcessNode& parentProcess) const override;

	private:
		std::vector<std::unique_ptr<BnfNode>> _children;
	};

}

#endif /*bnf_nodes_h*/