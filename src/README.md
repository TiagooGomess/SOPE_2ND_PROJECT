Este projeto retrata o funcionamento de um quarto de banho que constantemente recebe utilizadores que pretendem utilizá-lo. 

Relativamente ao desenvolvimento do código, temos apenas umas pequenas notas a acrescentar:

-> Um cliente apenas desiste de ir ao quarto de banho se após uma primeira leitura a mesma falhar e após 1.5s de espera com intervalos de 150ms entre novas tentativas de leitura.
Uma vez que não era mencionado nada específico no enunciado, essa foi a estratégia adotada.

-> O número de lugares máximo na casa de banho está limitado a 500 e o número máximo de Threads a 250.
