#include <ilcplex/ilocplex.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <stdio.h>
#include <time.h>

typedef struct timespec timespec_t;
ILOSTLBEGIN
#define BILLION 1000000000L

/*-----------------------------------------------------------------------------------------------------------------------------------------
    Declaracao de variaveis
-------------------------------------------------------------------------------------------------------------------------------------------
*/
IloEnv env;
int i, j, k, n, t, alpha; // contadores
int N; // número total de pedidos
int J; // número total de produtos
int T; // número total de períodos
double TiLim = 3600;

int myMin(int a, int b){
    if (a <= b) return a;
    else return b;
}

//double EpGap = 0.50;
ILOMIPINFOCALLBACK2(logTimeCallback,timespec_t, startTime, const string, nome)
{
  FILE *arq;
arq = fopen(nome.c_str(), "a");

    // Tempo atual
    timespec_t currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);

    // Tempo de execucao
    double execTime = (currTime.tv_sec - startTime.tv_sec) + (double)(currTime.tv_nsec - startTime.tv_nsec)/BILLION;

    if (!hasIncumbent()) {
        fprintf(arq, " %.2f %.2f\n", execTime, getBestObjValue());
    } else {
        fprintf(arq, " %.2f %.2f %.2f %.2f\n", execTime, getBestObjValue(), getIncumbentObjValue(),((getBestObjValue()-getIncumbentObjValue())/getBestObjValue())*100);
    }
    fclose(arq);
}
int main(int argc, char **argv){



try{

        // leitura dos dados
        ifstream entrada(argv[1]);
        entrada >> J;
        entrada >> T;
        entrada >> N;
        string nome(argv[2]);


        //demanda do item j no pedido n
        IloArray<IloNumArray> q(env, J);
		for ( j=0; j<J; ++j)
		{
        q[j] = IloNumArray (env,N);
		}
            for (n = 0; n < N; n++)
                for (j = 0; j < J; j++)
                entrada >> q[j][n];



        //custo de troca da produção do item i para o item j
        IloArray<IloNumArray> sc(env, J);
		for ( i=0; i<J; ++i)
		{
        sc[i] = IloNumArray (env,J);
		}
        //tempo de setup da produção do item i para o item j
        IloArray<IloNumArray> st(env, J);
		for ( i=0; i<J; ++i)
		{
        st[i] = IloNumArray (env,J);
		}
            for(i = 0; i < J; i++){
                for( j = 0; j < J; j++){
                entrada >> sc[i][j];
                entrada >> st[i][j];
                    }
                        }

		//primeiro período da janela de entrega do pedido n
       IloNumArray F(env, N);
         //último período da janela de entrego do pedido n
       IloNumArray L(env, N);
        for( n = 0; n < N; n++){
        entrada >> F[n];
        entrada >> L[n];
                }

         //capacidade (tempo) de produção do período t
        IloNumArray C(env, T);
        for (t = 0; t < T; t++)
        entrada >> C[t];


        //tempo de produção do item j
       IloNumArray a(env, J);
        for (j = 0; j < J; j++)
        entrada >> a[j];

        //custo de estoque do item j por unidade
       IloNumArray h(env, J);
        for (j = 0; j < J; j++)
        entrada >> h[j];

        //lucro associado ao pedido n no período t
        IloArray<IloNumArray> P (env, N);
		for ( n=0; n<N; ++n)
		{
		P[n] = IloNumArray(env,T);
		}
            for (n = 0; n < N; n++){
				for (t = 0; t < T; t++)
					entrada >> P[n][t];
                            }

        //tempo máximo de estoque do item j
       IloNumArray sl(env, J);
        for (j = 0; j < J; j++)
        entrada >> sl[j];



        IloModel model(env);


//// 1 se o pedido n vai ser atendido no período t e 0, caso contrário
        IloArray<IloBoolVarArray> gama(env, N);
		for ( n = 0 ; n < N ; n++ ){
			gama[n] = IloBoolVarArray(env, T);
                                }

//produção do item j no período t
            IloArray<IloNumVarArray> x(env, J);{
            for ( j = 0 ; j < J ; j++ )
            x[j] = IloNumVarArray(env, T, 0,IloInfinity);
            }
////estoque do item j com idade k ao final do período t
  IloArray<IloArray<IloNumVarArray> > I (env, J);
        for ( j=0; j<J; ++j) {
            I [j] = IloArray<IloNumVarArray> (env, sl[j]+2);
                for ( k=0; k <= sl[j] +1; k++) {
                    I [j][k] = IloNumVarArray (env, T+1, 0, IloInfinity);
                                }
                                    }

// quantidade do estoque j com idade k utilizada para atender o pedido n no período t
IloArray<IloArray<IloArray<IloNumVarArray> > > Q(env, J);
        for ( j = 0 ; j < J ; j++ ){
			Q[j] = IloArray<IloArray<IloNumVarArray> >(env, sl[j]+1);
			for ( k = 0 ; k <= sl[j] ; k++ ){
                Q[j][k] = IloArray<IloNumVarArray>(env, N);
				for ( n = 0 ; n < N ; n++ ){
					Q[j][k][n] = IloNumVarArray(env, T+1, 0, IloInfinity);
				}
                    }
                        }

//// 1 se a máquina está preparada para a produção do item j no início do período t e 0, caso contrário
        IloArray<IloBoolVarArray> y(env, J);
		for ( j = 0 ; j < J ; j++ ){
			y[j] = IloBoolVarArray(env, T+1);
                                }

//// 1 se ocorre troca da produção do item i para o item j durante o período t e 0, caso contrário
IloArray< IloArray<IloBoolVarArray> >  z(env, J);
            for ( i = 0 ; i < J ; i++ ){
			z[i] = IloArray<IloBoolVarArray> (env, J);
                for ( j = 0 ; j < J ; j++ ){
                z[i][j] = IloBoolVarArray(env, T);
                }
                    }


////variável auxiliar que representa a ordem de produção do item j no período t.
            IloArray<IloNumVarArray> V(env, J);{
            for ( j = 0 ; j < J ; j++ )
            V[j] = IloNumVarArray(env, T, 0,IloInfinity);
            }

//Funcao objetivo
IloExpr fo(env);

            for( n=0;n<N; n++){
                for( t= F[n];t<=L[n];t++){
                fo += P[n][t]*gama[n][t];
                            }
                                }

                for ( t = 0 ; t < T; t++ ){
                    for ( j = 0 ; j < J ; j++ ){

                        for ( k=0 ; k <= myMin(sl[j],t) ; k++ ){
                            fo += - h[j]*I[j][k][t];
                            }
                                }
                                    }


                for ( t = 0 ; t < T ; t++ ){
                    for ( i = 0 ; i < J ; i++){
                        for ( j = 0 ; j < J ; j++){
                        fo += - sc[i][j]*z[i][j][t];
                        }
                            }
                                }

model.add(IloMaximize(env, fo));
fo.end();


/////////////////////////////////////////////////////RESTRIÇÕES////////////////////////////////////////////////////

//Restrição 1
IloExpr Expr1(env);
IloExpr Expr2(env);
IloExpr Expr3(env);

for (t=0 ; t<T ; t++ ){

if(t==0){

    for (j=0 ; j<J ; j++ ){

        for (n=0 ; n<N ; n++ ){
        Expr1 += q[j][n] * gama[n][t];
            }
model.add (I[j][0][t] == x[j][t] - Expr1 ) ;
Expr1.end();
Expr1 = IloExpr(env);

    }
        }


else {
    for (j=0 ; j<J ; j++ ){

        for (n=0 ; n<N ; n++ ){
         Expr1 += q[j][n] * gama[n][t];
                }

                for (k=0 ; k<= myMin(sl[j],t) ; k++ ){
                Expr2 += I[j][k][t];
                }
                for (k=0 ; k<= myMin(sl[j],t-1) ; k++ ){
                Expr3 += I[j][k][t-1];
                }

model.add (Expr3 + x[j][t] == Expr1 + Expr2 ) ;
Expr1.end();
Expr1 = IloExpr(env);
Expr2.end();
Expr2 = IloExpr(env);
Expr3.end();
Expr3 = IloExpr(env);

    }
        }
          }

IloExpr Expr123(env);
for (t=0 ; t<T ; t++ ){
    for (j=0 ; j<J ; j++ ){
        for (n=0 ; n<N ; n++ ){
            Expr123 += Q[j][0][n][t];
            }
        model.add( x[j][t] - Expr123 == I[j][0][t]);
        Expr123.end();
        Expr123 = IloExpr(env);
        }
            }

// Definação das idades dos estoques
IloExpr Expr5(env);
for ( t = 1 ; t < T ; t++ ){
    for ( j = 0 ; j < J ; j++ ){
        for ( k = 1 ; k <= myMin(sl[j],t) ; k++ ){
    for ( n= 0 ; n < N; n++){
    Expr5 += Q[j][k][n][t] ;
        }
model.add ( I[j][k][t] == I[j][k-1][t-1] - Expr5 );
Expr5.end();
Expr5 = IloExpr(env);

        }
            }
                }

IloExpr Expr1234(env);
for ( t = 0 ; t < T ; t++ ){
    for ( j = 0 ; j < J ; j++ ){
        for ( n= 0 ; n < N; n++){
            for ( k = 0 ; k <= myMin(sl[j],t) ; k++ ){
            Expr1234+= Q[j][k][n][t];
            }
            model.add(Expr1234 == q[j][n]*gama[n][t]);
            Expr1234.end();
            Expr1234 = IloExpr(env);
            }
                }
                    }


for ( t = 0 ; t < T ; t++ ){
    for ( j = 0 ; j < J ; j++ ){
        model.add(I[j][sl[j]+1][t] == 0);
        }
            }

// Restrição 2
IloExpr Expr7(env);
        for ( t = 0 ; t < T ; t++ ){
			for ( i = 0 ; i < J ; i++ ){
				for ( j = 0 ; j < J ; j++ ){
						Expr7 += st[i][j]*z[i][j][t];
                                }
                        }
			for(j=0; j<J; j++){
			     Expr7 += a[j]*x[j][t] ;
			}
                model.add(Expr7 <= C[t] );
				Expr7.end();
				Expr7 = IloExpr(env);

        }
//   Restrição 3
IloExpr Expr8(env);
        for ( t = 0 ; t < T ; t++ ){
		for ( j = 0 ; j < J ; j++ ){
						Expr8 += y[j][t];
					}
						model.add(Expr8 == 1);
						Expr8.end();
                        Expr8 = IloExpr(env);
                        }

// Restrição 4
IloExpr Expr9(env);
IloExpr Expr10(env);
		for ( t = 0 ; t < T ; t++ ){
		for ( j = 0 ; j < J ; j++ ){
            for ( i = 0 ; i < J ; i++ ){
            if(i!=j){
			Expr9 += z[i][j][t] ;
			Expr10 += z[j][i][t] ;
			}
			}
			model.add(y[j][t] + Expr9 == Expr10 + y[j][t+1]);
            Expr9.end();
            Expr9 = IloExpr(env);
            Expr10.end();
			Expr10 = IloExpr(env);
		}

		}


        //Restrição 5
        for ( t = 0 ; t < T ; t++ ){
			for ( i = 0 ; i < J ; i++ ){
			if(j!=i){
			for ( j = 0 ; j < J ; j++ ){

				model.add(V[j][t] >= V[i][t] + 1 - J*(1 - z[i][j][t]));
                            }
			}
		}
		}

//  Restrição 7
    IloExpr Expr13(env);
        for ( t = 0 ; t < T ; t++ ){
			for ( j = 0 ; j < J ; j++ ){
                    for ( i = 0 ; i < J ; i++ ){
                            if(j!=i){
							Expr13 += z[i][j][t];
                                    }
                                        }
                            for ( i = 0 ; i < J ; i++ ){
                            model.add(x[j][t] <= (C[t]/a[j])*(y[j][t] + Expr13) -(st[i][j]/a[j])*Expr13) ;
                                                    }
                                Expr13.end();
                                Expr13 = IloExpr(env);
                                                    }
                                                            }

//Restrição 9
IloExpr Expr15(env);
        for ( n = 0 ; n < N ; n++ ){
						for( t = F[n] ; t <= L[n] ; t++ ){
							Expr15+= gama[n][t];
						}
						model.add(Expr15 <= 1);
                        Expr15.end();
                        Expr15 = IloExpr(env);

					}

  //Restrição 10
  for ( n = 0 ; n < N ; n++ ){
  for ( t = 0 ; t < T ; t++ ){
            if(t< F[n] || t > L[n])
				model.add(gama[n][t] == 0);
				}
				}



        IloCplex cplex(model);
        cplex.extract(model);
        //cplex.exportModel("modelo.lp");
        cplex.setParam(IloCplex::Param::TimeLimit,TiLim);
        //cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap,EpGap);

        IloModel relax(env);
        relax.add(model);

         for ( n = 0 ; n < N ; n++ ){
            for ( t = 0 ; t < T ; t++ ){
                relax.add(IloConversion(env, gama[n][t], ILOFLOAT));
                    }
                        }
            for ( j = 0 ; j < J ; j++ ){
            for ( t = 0 ; t < T+1 ; t++ ){
                relax.add(IloConversion(env, y[j][t], ILOFLOAT));
                    }
                        }
            for ( i = 0 ; i < J ; i++ ){
            for ( j = 0 ; j < J ; j++ ){
            for ( t = 0 ; t < T ; t++ ){
                relax.add(IloConversion(env, z[i][j][t], ILOFLOAT));
                    }
                        }
                            }
        IloCplex RELAX(relax);

    timespec_t startTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    cplex.use(logTimeCallback(env, startTime,nome));
    double tempo;

if(!cplex.solve()){
    env.error() << "Nao se pode resolver" << endl;
    throw(-1);
}
if(!RELAX.solve()){
    env.error() << "Nao se pode resolver" << endl;
    throw(-1);
}
double FO;
FO = cplex.getObjValue();
cout << "FO = " << FO << endl;

int Nodes;
double FOR;
Nodes = cplex.getNnodes ();
FOR = RELAX.getObjValue();
FILE *arq;
arq = fopen(argv[2], "a");
fprintf(arq, " Numero de nos explorados: %d\n", Nodes);
fprintf(arq, " RELAXACAO LINEAR: %.2f\n ", FOR);
fclose(arq);

} // fim do try

catch(IloException& ex) {
cerr << "ERRO: " << ex << endl;
}
catch(...) {
cerr << "ERRO!" << endl;
}

env.end();
return 0;

}
