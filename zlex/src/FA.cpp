#include "FA.hpp"

void FA::setOutputFile(std::string fileName)
{
    std::string folderPath = fileName.substr(0, fileName.find_last_of("/\\"));
    std::filesystem::create_directories(folderPath);
    outFile.open(fileName, std::ios::trunc);
    if (!outFile.is_open())
    {
        perror("打开文件失败");
    }

    // 向outFile写入当前时间
    std::time_t currentTime = std::time(nullptr);
    outFile << "Generate time: " << std::asctime(std::localtime(&currentTime));
}

void FA::printFA(int startStateID, FAStateVec &states)
{
    // 打印正则表达式集合
    if (!printRegexFlag)
    {
        outFile << "## 正则表达式集合" << std::endl;
        for (auto &regex : regexVec)
        {
            outFile << "- " << regex << std::endl;
        }
        printRegexFlag = true;
    }

    // 打印状态图
    if (states == NFAStates)
    {
        outFile << std::endl
                << "## NFA状态图" << std::endl;
    }
    else
    {
        outFile << std::endl
                << "## DFA状态图" << std::endl;
    }

    outFile << "```mermaid" << std::endl;
    outFile << "graph LR" << std::endl;

    std::stack<int> stateStack;
    std::unordered_map<int, bool> visited;
    stateStack.push(startStateID);
    // dfs 打印状态
    int i = 1;
    while (!stateStack.empty())
    {
        int stateID = stateStack.top();
        stateStack.pop();
        visited[stateID] = true;

        // print节点
        if (debugMode)
        {
            // print节点
            if (stateID == startStateID)
            {
                outFile << stateID << "((START<" << stateID << ">))" << std::endl;
            }
            else if (states[stateID].isAccepting)
            {
                outFile << stateID << "((END<" << stateID << ">))" << std::endl;
            }
            else
            {
                outFile << stateID << "((" << i++ << "<" << stateID << ">"
                        << "))" << std::endl;
            }
        }
        else
        {
            if (states[stateID].isAccepting)
            {
                outFile << stateID << "((END))" << std::endl;
            }
            else if (stateID == startStateID)
            {
                outFile << stateID << "((START))" << std::endl;
            }
            else
            {
                outFile << stateID << "((" << i++ << "))" << std::endl;
            }
        }

        // print符号边
        for (auto &trans : states[stateID].trans)
        {
            outFile << stateID << "--" << trans.first << "-->" << trans.second << std::endl;
            if (visited.find(trans.second) == visited.end())
            {
                stateStack.push(trans.second);
                visited[trans.second] = true;
            }
        }

        // print空边
        for (auto &epsilonTrans : states[stateID].epsilonTrans)
        {
            outFile << stateID << "--" << EPSILON_CHAR << "-->" << epsilonTrans << std::endl;
            if (visited.find(epsilonTrans) == visited.end())
            {
                stateStack.push(epsilonTrans);
                visited[epsilonTrans] = true;
            }
        }
        outFile << std::endl;
    }

    outFile << "```" << std::endl;
}

int FA::addState(bool isAccepting, FAStateVec &states)
{
    FAState faState(states.size(), {}, {}, isAccepting);
    states.push_back(faState);
    return states.size() - 1;
}

void FA::addEdge(int fromStateID, int toStateID, char symbol, FAStateVec &states)
{
    std::string str_sym(1, symbol);
    states[fromStateID].trans[str_sym] = toStateID;
}

void FA::addEdge(int fromStateID, int toStateID, std::string symbol, FAStateVec &states)
{
    if (symbol == EPSILON) // 空边
    {
        states[fromStateID].epsilonTrans.push_back(toStateID);
    }
    else // 转移条件
    {
        states[fromStateID].trans[symbol] = toStateID;
    }
}

// TODO 部分代码功能被regexToBlock替代 待重构
FAStateBlock FA::addTransition(char inputSymbol, bool escapeFlag, FAStateBlock block1, FAStateBlock block2, FAStateVec &states)
{
    FAStateBlock resultBlock{block1.beginStateID, block2.endStateID};
    // 问就是不爱用switch
    if (escapeFlag)
    {
        alphabet.insert(std::string(1, inputSymbol));
        int s1 = addState(0, states), s2 = addState(1, states);
        addEdge(s1, s2, inputSymbol, states);
        resultBlock = {s1, s2};
    }
    else if (inputSymbol == '-') // union
    {
        states[block1.endStateID].isAccepting = 0; // block1的终态取消
        addEdge(block1.endStateID, block2.beginStateID, EPSILON, states);
    }
    else if (inputSymbol == '|') // or
    {
        // block1,2的终态取消
        states[block1.endStateID].isAccepting = 0;
        states[block2.endStateID].isAccepting = 0;

        int orBegin = addState(0, states), orEnd = addState(1, states);
        addEdge(orBegin, block1.beginStateID, EPSILON, states);
        addEdge(orBegin, block2.beginStateID, EPSILON, states);
        addEdge(block1.endStateID, orEnd, EPSILON, states);
        addEdge(block2.endStateID, orEnd, EPSILON, states);
        resultBlock = {orBegin, orEnd};
    }
    else if (inputSymbol == '*') // 闭包
    {
        states[block1.endStateID].isAccepting = 0;
        int closureBegin = addState(0, states), closureEnd = addState(1, states);
        addEdge(closureBegin, block1.beginStateID, EPSILON, states);
        addEdge(block1.endStateID, closureEnd, EPSILON, states);
        addEdge(closureBegin, closureEnd, EPSILON, states);
        addEdge(block1.endStateID, block1.beginStateID, EPSILON, states);
        resultBlock = {closureBegin, closureEnd};
    }
    else if (inputSymbol == '+') // 正闭包
    {
        int closureBegin = addState(0, states), closureEnd = addState(1, states);
        addEdge(closureBegin, block1.beginStateID, EPSILON, states);
        addEdge(block1.endStateID, closureEnd, EPSILON, states);
        addEdge(block1.endStateID, block1.beginStateID, EPSILON, states);
        resultBlock = {closureBegin, closureEnd};
    }
    else if (inputSymbol == '?') // 零次或一次
    {
        int closureBegin = addState(0, states), closureEnd = addState(1, states);
        addEdge(closureBegin, block1.beginStateID, EPSILON, states);
        addEdge(block1.endStateID, closureEnd, EPSILON, states);
        addEdge(closureBegin, closureEnd, EPSILON, states);
        resultBlock = {closureBegin, closureEnd};
    }
    else // 普通字符
    {
        alphabet.insert(inputSymbol + "");
        int s1 = addState(0, states), s2 = addState(1, states);
        addEdge(s1, s2, inputSymbol, states);
        resultBlock = {s1, s2};
    }
    return resultBlock;
}

std::string FA::addUnion(std::string regex)
{
    std::string result;
    for (int i = 0; i < regex.size(); i++)
    {
        if (regex[i] == '\\' && !Symbol::isOperator(regex[i + 2]) && (i + 2) != regex.size())
        {
            // 转义字符
            result += regex[i++];
            result += regex[i];
            result += '-';
        }
        // else if ((!Symbol::isOperator(regex[i])) && !Symbol::isOperator(regex[i + 1]) && (i + 1) != regex.size())
        // TODO 好丑陋的if 改了它!
        else if ((regex[i] != '|' && regex[i] != '(') && (!Symbol::isOperator(regex[i + 1]) || regex[i + 1] == '(') && (i + 1) != regex.size())
        {
            result += regex[i];
            result += '-';
        }
        else
        {
            result += regex[i];
        }
    }
    return result;
}

std::string FA::infixToSufix(std::string regex)
{
    std::stack<char> opStack;
    std::string res;

    for (int i = 0; i < regex.size(); i++)
    {
        // 转义字符
        if (regex[i] == '\\')
        {
            res += '\\';
            res += regex[++i];
        }
        else if (regex[i] == '(')
        {
            opStack.push(regex[i]);
        }
        else if (regex[i] == ')')
        {
            while (opStack.top() != '(')
            {
                res += opStack.top();
                opStack.pop();
            }
            // 弹出左括号
            opStack.pop();
        }
        else if (Symbol::isOperator(regex[i]))
        {
            while (!opStack.empty() && Symbol::opOrder[opStack.top()] <= Symbol::opOrder[regex[i]])
            {
                res += opStack.top();
                opStack.pop();
            }
            opStack.push(regex[i]);
        }
        else // 普通字符
        {
            res += regex[i];
        }
    }
    while (!opStack.empty())
    {
        res += opStack.top();
        opStack.pop();
    }

    return res;
}

FAStateBlock FA::regexToBlock(std::string regex, FAStateVec &states)
{
    std::string regexWithUnion = addUnion(regex);
    std::string regexWithSuffix = infixToSufix(regexWithUnion);

    std::stack<FAStateBlock> blockStack;
    for (int i = 0; i < regexWithSuffix.size(); i++)
    {
        FAStateBlock block1, block2;
        if (regexWithSuffix[i] == '\\')
        {
            if (i + 1 > regexWithSuffix.size())
            {
                perror("无效的转义字符");
            }
            blockStack.push(addTransition(regexWithSuffix[i + 1], 1, {}, {}, states));
        }
        else if (Symbol::isOperator(regexWithSuffix[i]))
        {
            block2 = blockStack.top();
            // 正则操作符对象可能只有一个
            if (!Symbol::isUnaryOp(regexWithSuffix[i]))
            {
                blockStack.pop();
            }
            block1 = blockStack.top();
            blockStack.pop();
            blockStack.push(addTransition(regexWithSuffix[i], 0, block1, block2, states));
        }
        else
        {
            // 直连
            // alphabet.insert(regexWithSuffix[i] + "");
            alphabet.insert(std::string(1, regexWithSuffix[i]));
            // symbolTable[regexWithSuffix[i] + ""] = true; // 记录符号
            blockStack.push({addState(0, states), addState(1, states)});
            addEdge(blockStack.top().beginStateID, blockStack.top().endStateID, regexWithSuffix[i], states);
        }
    }
    return blockStack.top();
}

void FA::buildNFA(RegexVec regexVec)
{
    this->regexVec = regexVec;
    int allBegin = addState(0, NFAStates);
    for (auto &regex : regexVec)
    {
        FAStateBlock block = regexToBlock(regex, NFAStates);
        addEdge(allBegin, block.beginStateID, EPSILON, NFAStates);
    }
    NFAStartStateID = allBegin;

    if (debugMode)
    {
        outFile << std::endl
                << "## 字母表" << std::endl;
        for (auto &letter : alphabet)
        {
            outFile << letter << " ";
        }
        outFile << std::endl;
    }
}

/**
 * @brief 获取一个状态的epsilon闭包
 * @param stateID 状态ID
 * @param visited 访问标记(用于避免重复访问)
 */
StateSet FA::epsilonClosure(int stateID, std::unordered_map<int, bool> &visited, FAStateVec &states)
{
    std::stack<int> stateStack;
    stateStack.push(stateID);
    StateSet result;
    while (!stateStack.empty())
    {
        int stateID = stateStack.top();
        stateStack.pop();
        result.set.insert(stateID);
        visited[stateID] = true;
        for (auto &epsilonTrans : states[stateID].epsilonTrans)
        {
            if (visited.find(epsilonTrans) == visited.end())
            {
                stateStack.push(epsilonTrans);
                result.isAccepting = result.isAccepting || states[epsilonTrans].isAccepting;
                visited[epsilonTrans] = true;
            }
        }
    }
    return result;
}

/**
 * @brief 获取一个状态集的epsilon闭包
 * @param stateSet 状态集
 * @param visited 访问标记(用于避免重复访问)
 */
StateSet FA::epsilonClosure(StateSet stateSet, std::unordered_map<int, bool> &visited, FAStateVec &states)
{
    StateSet result;
    for (auto &stateID : stateSet.set)
    {
        StateSet closure = epsilonClosure(stateID, visited, states);
        result.set.insert(closure.set.begin(), closure.set.end());
        result.isAccepting = result.isAccepting || closure.isAccepting;
    }
    return result;
}

/**
 * @brief 获取一个状态集的转移闭包
 * @param stateSet 状态集
 * @param symbol 转移条件
 */
StateSet FA::move(StateSet stateSet, std::string symbol, FAStateVec &states)
{
    StateSet result;
    // 遍历状态集
    for (auto &stateID : stateSet.set)
    {
        // 查看当前状态是否有symbol转移
        if (states[stateID].trans.find(symbol) != states[stateID].trans.end())
        {
            result.set.insert(states[stateID].trans[symbol]);
            result.isAccepting = result.isAccepting || states[stateID].isAccepting;
        }
    }
    return result;
}

void FA::printDFATransTableHeader()
{
    outFile << std::endl
            << "## NFA->DFA(子集构造)" << std::endl;
    outFile << "|原NFA集合|";
    for (auto &symbol : alphabet)
    {
        outFile << symbol << "|";
    }
    outFile << std::endl;
    // 打印分隔线
    outFile << "|---|";
    for (auto &symbol : alphabet)
    {
        outFile << "---|";
    }
    outFile << std::endl;
}

/**
 * @brief 将states中的NFA转换为DFA
 */
void FA::toDFA()
{
    if (debugMode)
        printDFATransTableHeader();

    std::unordered_set<StateSet, StateSetHash, StateSetEqual> visitedSet;
    std::queue<StateSet> stateSetQueue;

    StateSet startStateSet = epsilonClosure(NFAStartStateID, NFAStates);
    // 初始NFA状态集->DFA状态
    startStateSet.stateID = addState(startStateSet.isAccepting, DFAStates);
    DFAStartStateID = startStateSet.stateID;

    // 开始递归 生成DFA
    stateSetQueue.push(startStateSet);
    while (!stateSetQueue.empty())
    {
        // 获取当前状态集
        StateSet currentStateSet = stateSetQueue.front();
        stateSetQueue.pop();

        // 跳过已访问的状态集
        if (visitedSet.find(currentStateSet) != visitedSet.end())
            continue;

        visitedSet.insert(currentStateSet);

        if (debugMode)
        {
            outFile << "|" << currentStateSet << "|";
        }

        for (auto &symbol : alphabet)
        {
            // TODO 生成的nextStateSet的StateID从何而来
            StateSet nextStateSet = epsilonClosure(move(currentStateSet, symbol, NFAStates), NFAStates);
            // 跳过空集
            if (nextStateSet.set.size() == 0)
            {
                if (debugMode)
                    outFile << EMPTY_SET << "|";
                continue;
            }

            // 检查是否为新生成的集合(DFA状态)
            if (visitedSet.find(nextStateSet) == visitedSet.end())
            {
                // visitedSet.insert(nextStateSet);
                nextStateSet.stateID = addState(nextStateSet.isAccepting, DFAStates);
                stateSetQueue.push(nextStateSet);
            }
            else
            {
                // 重复状态集
                // NOTE: 否则StateID会因为epsilonClosure生成出错
                nextStateSet = *visitedSet.find(nextStateSet);
            }

            if (debugMode)
            {
                outFile << nextStateSet << "|";
            }
            // 为新生成的状态集(DFA状态)添加转移(边)
            addEdge(currentStateSet.stateID, nextStateSet.stateID, symbol, DFAStates);
        }

        if (debugMode)
            outFile << std::endl;
    }
}