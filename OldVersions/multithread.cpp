#include <iostream>
#include <string>
#include <fstream>

using namespace std;

int main(int argc, char **argv)
{
    char *filename;
    if (argc == 2)
    {
        filename = strdup(argv[1]);
    }
    else
    {
        filename = strdup("input.txt");
    }
    ifstream file(filename, ios::in); // on ouvre en lecture

    if (file) // si l'ouverture a fonctionné
    {
        string contenu;
        getline(file, contenu);
        ofstream outputFile("output.txt", ios::out | ios::app);
        if (outputFile)
        {
            outputFile << contenu << endl;
        }
        cout << contenu;
        file.close();
    }
    else
        cerr << "Impossible d'ouvrir le fichier !" << endl;

    return 0;
}
