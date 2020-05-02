Este projeto retrata o funcionamento de um quarto de banho que constantemente recebe utilizadores que pretendem utilizá-lo. 

Relativamente ao desenvolvimento do código, temos apenas duas pequenas notas a acrescentar:

-> Um cliente apenas desiste de ir ao quarto de banho se após uma primeira leitura a mesma falhar e após 90ms de espera e posterior nova leitura a mesma também falhar.
Uma vez que não era mencionado nada específico no enunciado, essa foi a estratégia adotada.

-> Relativamente ao servidor, o mesmo recebe os pedidos da FIFO pública utilizando a flag O_NONBLOCK o que praticamente impossibilita a existência de 2LATES tirando em situações excecionais.
Deste modo após uma leitura, o servidor espera 25 ms (mesmo tempo entre pedidos do cliente) entre cada leitura.
