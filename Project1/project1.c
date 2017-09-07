#include <stdio.h>
#include <project1.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

char *readLine(){
char *input;
int bytes_read;
size_t n = 80;
input = malloc (sizeof(char) * n);
bytes_read = getline(&input,&n, stdin); 
if (bytes_read==-1)
	return "ERROR";
else{
	input[strlen(input)-1] = '\0'; //deleting new line from input
	return input;
}
}

char ** splitLine(char *input){
char **ret = malloc(sizeof(char*) * 80);
char *token;
int i=0;
int length = strlen(input)-1;
const char *DELIMETER = " ";
token = strtok(input, DELIMETER);
while(token!=NULL){
	ret[i] = token;
	token = strtok(NULL, DELIMETER);
	i++;
}
return ret;
}

#define INVALID_CHAR 1
#define SPACES_NEEDED 2
#define NEED_WORD 3
#define INPUT_OUTPUT 4
#define OPERATOR_FIRSTORLAST 5
#define OUTPUT_PIPE_ORDER 6
#define DOUBLE_OPERATOR 7
#define NEED_WORD_BETWEENPIPES 8
int execute(char **array){
char charset[] = "0123456789-_.abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ<>|";
char opset[] = "<>|";
size_t spn;
int i=0, length=0, infiles=0, outfiles=0, pipes=0;
char *token;
while(array[i]!=NULL){
	token = array[i];
	length = strlen(token);
	spn = countChar(token, opset); 
//checking for ops first
	if(strcmp(token, "<<") == 0 || strcmp(token, ">>") == 0)
		return DOUBLE_OPERATOR;
	if(spn!=0 && i==0 && length == 1)
		return NEED_WORD;
	if(length > 1 && spn !=0 && length!=spn)
		return SPACES_NEEDED;
	if(spn>1)
		return SPACES_NEEDED;
	if(pipes>0 && strcmp(token, "|")==0 && strcmp(array[i-1], "|")==0)
		return NEED_WORD_BETWEENPIPES;
		

///NOW CHECKING WORDS//
	spn = strspn(token, charset); // checking validity of words
	if(length!=spn)
		return INVALID_CHAR;
	if(   (strcmp(token,"<")==0 || strcmp(token,">")==0 || strcmp(token,"|")==0) && (i==0 || array[i+1]==NULL))
		return OPERATOR_FIRSTORLAST; //cannot have an operator as the first or last token
	if(strcmp(token,"<")==0){
		
		if(infiles > 0 || outfiles > 0)
			return INPUT_OUTPUT;
		infiles++;
	}
	if(strcmp(token,">")==0){
		if(outfiles>0)
			return INPUT_OUTPUT;
		outfiles++;
	}
	if(strcmp(token,"|")==0){
		if(outfiles>0)
			return OUTPUT_PIPE_ORDER;
		pipes++;
	}

	i++;
}

if(pipes==0)
	runNopipes(array, infiles, outfiles);
else
	run(array, infiles, outfiles, pipes);

return 0;
}

int getInfile(char **args){
int i=0;
	while(args[i]!=NULL){
		if(strcmp(args[i], "<")==0)
			return (i+1);
		i++;
	}
return 0;
}

int getOutfile(char **args){
int i=0;
	while(args[i]!=NULL){
		if(strcmp(args[i], ">")==0)
			return (i+1);
		i++;
	}
return 0;
}

char **inFileArray(char **args, int loc){
	char ** ret = malloc(80 * sizeof(char));
	int i;
	for(i=0; i<loc-1; i++){
		ret[i] = args[i];
	}
	return ret;

}
//returns the nth token group separated by a |
char **getTg(char **args, int n){
	char **ret = malloc(80 * sizeof(char));
	int begPipe=0;
	int endPipe=0;
	if(n>0)
		begPipe = getPipeloc(args, n);
	endPipe = getPipeloc(args, n+1);
	int i=0;
	if(begPipe!=0) begPipe++;
	while(begPipe < endPipe){
		ret[i]=args[begPipe];
		begPipe++;
		i++;
	}
	ret[i] = NULL;
	return ret;

}
//returns the index of the nth pipe in the array
int getPipeloc(char **args, int pipenum){
	int i=0;
	int pipesFound=0;
	while(args[i]!=NULL){
		if(strcmp(args[i], "|")==0){
			pipesFound++;
			if(pipesFound==pipenum)
			return i;
		}
	i++;
		
	}
	return i;

}
void run(char **args, int infiles, int outfiles, int pipes){
	int tgs = pipes + 1;
	int i=0;
	int status, in, out, infileLoc, outfileLoc;
	int pipefd[2*pipes];
	char **inArgs = malloc(80 * sizeof(char));
	char **tempArgs = malloc(80 * sizeof(char));
	if(infiles){
		infileLoc = getInfile(args);
		if((in = open(args[infileLoc], O_RDONLY)) < 0){
				printf("ERROR: Could not open the file.\n");
				return;
				//exit(0);
			}
		inArgs = inFileArray(args, infileLoc);
	}
	if(outfiles){
			outfileLoc = getOutfile(args);
			if((out = open(args[outfileLoc], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) < 0){
				printf("ERROR: Could not create the file.\n");
				return;
				//exit(0);
			}
	}
	for(i=0;i<pipes;i++)
		pipe(pipefd + (2*i));

	pid_t pid;
	int j=0;
	int k=0;
	int s=1;
	int offset;
	int tgLoc[tgs+1];
	int infileCom = -1;
	int outfileCom = -1;
	tgLoc[0] = 0;
	while(args[k]!=NULL){
		if(strcmp(args[k], "<")==0){
			infileCom = s-1;
		}
		if(strcmp(args[k], ">")==0){
			outfileCom = s-1;
		}
		if(strcmp(args[k], "|")==0){
			args[k] = NULL;
			tgLoc[s] = k+1;
			s++;
		}
		k++;
	}
	for(i=0; i<tgs; i++){
		offset = tgLoc[i];
		pid = fork();
		if(pid == 0){
			if(i == outfileCom){
				dup2(out, 1);
				close(pipefd[1]);
			}
			else
				dup2(pipefd[j+1], 1);
			if(i == infileCom){
					dup2(in, 0);
					close(pipefd[0]);
			}
			else
			if(j !=0 ) {	
				
					dup2(pipefd[j-2], 0);
			}

			int q;
			for(q=0; q<2*pipes; q++)
				close(pipefd[q]);
			if(i==infileCom)
				execvp(inArgs[0], inArgs);
			else
				if(i == outfileCom){
					tempArgs = inFileArray(args+offset, outfileLoc-offset);
					//printf("Tempargs is \n");
					//printTokens(tempArgs);
					execvp(tempArgs[0], tempArgs);
				}
				else
					execvp(args[offset], args+offset);
		}
		j+=2;
		
	}
	for(i=0; i < 2*pipes; i++)
		close(pipefd[i]);
	for(i=0; i<pipes+1; i++)
		wait(&status);

}
void closePipes(int pipes[]){
	int i=0;
	while(pipes[i]!=NULL)
		close(pipes[i]);
}
void runNopipes(char **args, int infiles, int outfiles, int pipes){
	pid_t pid;
	int in, out; //FD of in/out files
	char **inArgs = malloc(80 * sizeof(char));

	pid = fork();
	if(pid==0) // child process
	{
		if(infiles){
			int infLoc = getInfile(args);
			if((in = open(args[infLoc], O_RDONLY)) < 0){
				printf("ERROR: Could not open the file.\n");
				exit(0);
			}
			dup2(in, 0); //replacing standard input with input file
			inArgs = inFileArray(args, infLoc);
			
			
		}
		if(outfiles){
			int outLoc = getOutfile(args);
			if((out = open(args[outLoc], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) < 0){
				printf("ERROR: Could not create the file.\n");
				exit(0);
			}
			dup2(out, 1);//replacing standard output with the output file
			if(infiles==0)
				args = inFileArray(args, outLoc);
			
		}
		close(in);
		close(out);
		if(infiles)
			execvp(inArgs[0], inArgs);
		else
			execvp(args[0], args);
		printf("ERROR: Error executing execvp.\n");
	}
	else if(pid<0)
		printf("ERROR: Error in creating child process.\n");
	else //parent process
	{
		wait(&pid);
	}
}
int countChar(const char *input, const char *chars){
int ret=0;
int len = strlen(chars);
int lenInput = strlen(input);
char temp ;
int i,y;
for(i=0; i<len; i++){
	temp = chars[i];
	for(y=0; y<lenInput; y++){
		if(input[y] == temp)
			ret++;
	}
}

return ret;
}
// PRINT STATEMENTS
void printTokens(char **array){
int i=0;
	while(array[i]!=NULL){
		printf("array [%i] = %s\n", i, array[i]);
		i++;
	}
}
void printError(int code){
	switch(code){
		case 1:
			printf("ERROR: Illegal character found in line.\n");
			break;
		case 2:
			printf("ERROR: Spaces required on either side of an operator ('<', '>', '|')\n"); 
			break;
		case 3:
			printf("ERROR: Need word, not operator, for the first token.\n");
			break;
		case 4:
			printf("ERROR: You are allowed at most one infile operator '<' followed by at most one outfile operator'>'.\n");
			break;
		case 5:
			printf("ERROR: You cannot have an operator ('<', '>', '|') as the first or last token.\n");
			break;
		case 6:
			printf("ERROR: An output redirect operator ('>') cannot be followed by a pipe ('|').\n");
			break;
		case 7:
			printf("ERROR: Two input or output redirection operators cannot be next to each other.\n");
			break;
		case 8:
			printf("ERROR: You need a word in between two pipe operators.\n");
			break;
	}
}

int main()
{
	char *input = "";
	char **tokens;
	int errorCode = 0;
	printf("$: ");
	input = readLine(); // sets input to be the read command
	while(strcmp(input, "exit")!=0)
	{
		if(strlen(input) > 80){
			printf("ERROR: line longer than 80 characters\n");
		}
		else {
			
			tokens = splitLine(input); //splits the input into an array of strings	
			//printTokens(tokens);
			errorCode = execute(tokens);
			if(errorCode!=0)
				printError(errorCode);
		}
	printf("$: ");
	input = readLine(); // sets input to be the read command
	if(strcmp(input, "exit")==0)
		return 0;
	}
	return 0;
}
