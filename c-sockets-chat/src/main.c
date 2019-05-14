#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "chatConnection.h"

#define STRILEN 30

bool isValidArg(char*);
//void printHelpInfoGer(void);
//void printHelpInfo(void);

int main()
{

    while(1)
    {
        char* linebuf = NULL;
        size_t linesize = 0;

        if(getline(&linebuf, &linesize, stdin) == -1)
        {
            perror("Failed to get input");
            fflush(stderr);
            free(linebuf);
            linebuf = NULL;
            return -1;
        }

        size_t linebuflen = strlen(linebuf);
        if(linebuf[linebuflen - 1] == '\n')
        {
            linebuf[linebuflen - 1] = '\0';
            //This check only required for Windows
            if(linebuf[linebuflen - 2] == '\r')
            {
                linebuf[linebuflen - 2] = '\0';
            }
            //--

        }
        fflush(stdout);

        char* nexttoken = NULL;
        char* originalmsg = malloc(strlen(linebuf));
        strcpy(originalmsg, linebuf);
        nexttoken = strtok(linebuf, " ");

        if(strcmp(linebuf, "connect") == 0)
        {
            nexttoken = strtok(NULL, " ");
            if(isValidArg(nexttoken))
            {
                connectToPeer(nexttoken);
            }
            else
            {
                fprintf(stderr, "Not enough arguments\n");
                fflush(stderr);
            }
        }
        else if(strcmp(linebuf, "disconnect") == 0)
        {
            disconnectFromPeer();
        }
        else if(strcmp(linebuf, "say") == 0)
        {
            nexttoken = strtok(NULL, " ");
            if(isValidArg(nexttoken))
            {
                sayToPeer(originalmsg + 4);
            }
            else
            {
                fprintf(stderr, "Not enough arguments\n");
                fflush(stderr);
            }
        }
        else if(strcmp(linebuf, "send") == 0)
        {
            nexttoken = strtok(NULL, " ");

            if(isValidArg(nexttoken))
            {
                sendToPeer(nexttoken);
            }
            else
            {
                fprintf(stderr, "Not enough arguments\n");
                fflush(stderr);
            }
        }
        else if(strcmp(linebuf, "host") == 0)
        {
            free(linebuf);
            linebuf = NULL;
            hostConnection();
        }
        else if(strcmp(linebuf, "quit") == 0)
        {
            free(linebuf);
            linebuf = NULL;
            quit();
        }
//        else if(strcmp(linebuf, "hilfe") == 0)
//        {
//            free(linebuf);
//            printHelpInfoGer();
//        }
//        else if(strcmp(linebuf, "help") == 0)
//        {
//            free(linebuf);
//            printHelpInfo();
//        }
        else
        {
            puts("Unknown command");
            fflush(stdout);
        }
        free(linebuf);
        linebuf = NULL;
        free(originalmsg);
        originalmsg = NULL;
    }
}

/*
 * Check, whether the passed string is a valid command line arg
 * (not null and strlen(str) > 0)
 */
bool isValidArg(char* str)
{
    return ((str != NULL) && (strlen(str) > 0));
}

//void printHelpInfoGer()
//{
//    char commandname[STRILEN] = "Befehl";
//    printf("%-20s",commandname); puts("Effekt");
//    printf("________________________________________________\n");
//    char connecti[STRILEN] = "connect <address>";
//    printf("%-20s",connecti); puts("Eine Verbindung zu <address> aufbauen. Dabei verhält sich die lokale Maschine als Client. Adressen in IPv4- und IPv6-Format sind erlaubt.");
//    char disconnecti[STRILEN] = "disconnect";
//    printf("%-20s",disconnecti); puts("Falls eine offene Verbindung besteht wird diese geschlossen. Danach kann eine neue Verbindung hergestellt werden.");
//    char sayi[STRILEN] = "say <nachricht>";
//    printf("%-20s", sayi); puts("Eine Textnachricht an den die Maschine am anderen Ende schicken.");
//    char sendi[STRILEN] = "send <dateipfad>";
//    printf("%-20s", sendi); puts("Eine Datei an die Maschine am anderen Ende schicken. Angabe eines absoluten Pfads ist empfohlen.");
//    char hosti[STRILEN] = "host";
//    printf("%-20s",hosti); puts("Bereitet diese Maschine als Host vor, mit dem sich andere Maschinen verbinden können.");
//    char quiti[STRILEN] = "quit";
//    printf("%-20s", quiti); puts("Schließt alle offenen Verbindungen und beendet das Programm");
//    char helpi[STRILEN] = "help";
//    printf("%-20s",helpi); puts("Zeigt dieses Hilfefenster in Englisch an");
//    char hilfi[STRILEN] = "hilfe";
//    printf("%-20s",hilfi); puts("Zeigt dieses Hilfefenster an");
//}
//
//void printHelpInfo()
//{
//    char* commandname = "Command";
//    printf("%-20s",commandname); puts("Effect");
//    printf("________________________________________________\n");
//    char* connecti = "connect <address>";
//    printf("%-20s",connecti); puts("Connect to <address> whereas this machine acts as a client. Numeric addresses in IPv4 and IPv6 format are valid.");
//    char* disconnecti = "disconnect";
//    printf("%-20s",disconnecti); puts("If a connections is present, it is discarded. New connections can be established afterwards");
//    char* sayi = "say <nachricht>";
//    printf("%-20s", sayi); puts("Send a text message to peer.");
//    char* sendi = "send <dateipfad>";
//    printf("%-20s", sendi); puts("Send a file to peer. Absolute paths are recommended");
//    char* hosti = "host";
//    printf("%-20s",hosti); puts("Prepares this machine as a host other machines can connect to");
//    char* quiti = "quit";
//    printf("%-20s", quiti); puts("Disconnect and shutdown.");
//    char* helpi = "help";
//    printf("%-20s",helpi); puts("Prints this help info.");
//    char* hilfi = "hilfe";
//    printf("%-20s",hilfi); puts("Prints this help info in German.");
//}

