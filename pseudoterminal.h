#ifndef PSEUDOTERMINAL_H
#define PSEUDOTERMINAL_H

 #include <string>

using namespace std;

class PseudoTerminal
{
public:
    /**
     * @brief Constructor: Setup linux pseudo terminal
     */
    PseudoTerminal();

    /**
     * @brief Destructor: Close pseudo terminal
     */
    ~PseudoTerminal();

    /**
     * @brief Path of the pseudo terminal
     * @return Path as string
     */
    string getPath();

    /**
     * @brief Will wait for input of a char
     * @return line as string
     */
    string readChar();

    /**
     * @brief Write data to pseudo terminal
     */
    void writeLine(string str);

private:
    string path;
    int masterfd;
};

#endif // PSEUDOTERMINAL_H
