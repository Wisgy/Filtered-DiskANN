#include <algorithm>
#include <functional>
#include <stack>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <vector>

template <typename T> class SyntaxTree
{
  private:
    // nodes
    template <typename U> class Node
    {
      public:
        virtual ~Node<U>()
        {
        }
        virtual bool check(const std::vector<U> *labels) = 0;
    };

    template <typename U> class OrNode : public Node<U>
    {
      private:
        Node<U> *_left_node, *_right_node;

      public:
        OrNode<U>(Node<U> *l, Node<U> *r) : _left_node(l), _right_node(r)
        {
        }
        ~OrNode<U>()
        {
            delete _left_node;
            delete _right_node;
        }

        bool check(const std::vector<U> *labels) override final
        {
            if (_left_node->check(labels))
            {
                return true;
            }
            else
            {
                return _right_node->check(labels);
            }
        };
    };
    template <typename U> class AndNode : public Node<U>
    {
      private:
        Node<U> *_left_node, *_right_node;

      public:
        AndNode<U>(Node<U> *l, Node<U> *r) : _left_node(l), _right_node(r)
        {
        }
        ~AndNode<U>()
        {
            delete _left_node;
            delete _right_node;
        }

        bool check(const std::vector<U> *labels) override final
        {
            if (not _left_node->check(labels))
            {
                return false;
            }
            else
            {
                return _right_node->check(labels);
            }
        };
    };
    template <typename U> class NotNode : public Node<U>
    {
      private:
        Node<U> *_sub_node;

      public:
        NotNode<U>(Node<U> *node) : _sub_node(node)
        {
        }
        ~NotNode<U>()
        {
            delete _sub_node;
        }

        bool check(const std::vector<U> *labels) override final
        {
            return not _sub_node->check(labels);
        };
    };
    template <typename U> class LabelNode : public Node<U>
    {
      private:
        U _label;

      public:
        LabelNode<U>(U label) : _label(label)
        {
        }
        ~LabelNode<U>()
        {
        }

        bool check(const std::vector<U> *labels) override final
        {
            return std::find(labels->begin(), labels->end(), _label) != labels->end();
        }
    };

    // constructed helper functions
    std::vector<std::string> parse_logic_expression(const std::string &logic_expr)
    {
        std::vector<std::string> tokens;
        std::string cur_token;

        for (char c : logic_expr)
        {
            if (c == ' ' || c == '\t')
            {
                continue;
            }
            else if (c == '|' || c == '&' || c == '!' || c == '(' || c == ')')
            {
                if (!cur_token.empty())
                {
                    tokens.push_back(cur_token);
                    cur_token.clear();
                }
                tokens.push_back(std::string(1, c));
            }
            else if (c >= '0' && c <= '9')
            {
                cur_token += c;
            }
            else
            {
                throw std::logic_error{"Token is incorrect"};
            }
        }

        if (!cur_token.empty())
        {
            tokens.push_back(cur_token);
        }

        return tokens;
    }

    bool is_operator(const std::string &token)
    {
        static const std::unordered_map<std::string, bool> operators{
            {"|", 1}, {"&", 1}, {"!", 1}, {"(", 1}, {")", 1},
        };

        return operators.find(token) != operators.end();
    };

    bool higher_precedence(const std::string &op1, const std::string &op2)
    {
        static const std::unordered_map<std::string, int> precedence{{"|", 1}, {"&", 2}, {"!", 3}};

        return precedence.at(op1) >= precedence.at(op2);
    };

    std::vector<std::string> convert2rpn(const std::vector<std::string> &&oper_tokens)
    {
        std::vector<std::string> rpn_expr;
        std::stack<std::string> oper_stack;

        for (const std::string &token : oper_tokens)
        {
            if (!is_operator(token))
            {
                rpn_expr.push_back(token);
            }
            else if (token == "(")
            {
                oper_stack.push(token);
            }
            else if (token == ")")
            {
                while (!oper_stack.empty() && oper_stack.top() != "(")
                {
                    rpn_expr.push_back(oper_stack.top());
                    oper_stack.pop();
                }
                if (!oper_stack.empty() && oper_stack.top() == "(")
                {
                    oper_stack.pop();
                }
            }
            else
            {
                while (!oper_stack.empty() && oper_stack.top() != "(" && higher_precedence(oper_stack.top(), token))
                {
                    rpn_expr.push_back(oper_stack.top());
                    oper_stack.pop();
                }
                oper_stack.push(token);
            }
        }

        while (!oper_stack.empty())
        {
            auto token = oper_stack.top();
            rpn_expr.push_back(token);
            oper_stack.pop();
        }
        return rpn_expr;
    }

    Node<T> *rpn2tree(const std::vector<std::string> &&rpn)
    {
        std::stack<Node<T> *> res_nodes{};
        for (auto &op : rpn)
        {
            if (!is_operator(op))
            {
                auto val = std::stoll(op);
                auto leaf_node = new LabelNode<T>((T)val);
                res_nodes.push(leaf_node);
            }
            else if (op == "|")
            {
                auto rhs = res_nodes.top();
                res_nodes.pop();
                auto lhs = res_nodes.top();
                res_nodes.pop();
                res_nodes.push(new OrNode<T>(lhs, rhs));
            }
            else if (op == "&")
            {
                auto rhs = res_nodes.top();
                res_nodes.pop();
                auto lhs = res_nodes.top();
                res_nodes.pop();
                res_nodes.push(new AndNode<T>(lhs, rhs));
            }
            else
            {
                auto sub_node = res_nodes.top();
                res_nodes.pop();
                res_nodes.push(new NotNode<T>(sub_node));
            }
        }
        if (res_nodes.size() != 1)
        {
            throw std::logic_error{"Extra label"};
        }
        return res_nodes.top();
    }

    Node<T> *root;

  public:
    explicit SyntaxTree<T>(const std::string &str_logic_expr)
    {
        auto tokens = parse_logic_expression(str_logic_expr);
        auto rpn = convert2rpn(std::move(tokens));
        root = rpn2tree(std::move(rpn));
    }
    ~SyntaxTree<T>()
    {
        delete root;
    }

    bool check(const std::vector<T> *labels)
    {
        return root->check(labels);
    }
};