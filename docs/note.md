PCIe DMA手册17页，Legacy interrupts，PL使得中断置位user_irq_req，之后IP将发出user_irq_ack，表示已将此中断发送给PCIe host，之后host完成中断事务后复位PL内**有关中断的寄存器**，PL判断user_irq_ack  assert且**有关中断的寄存器**也改变了状态，表明host已处理完中断，则复位user_irq_req

user_irq_req需要保持置位直到user_irq_ack发出且中断事务结束。

目前中断信号控制不完善。使用寄存器控制，主机轮询是否TX DONE即可。

添加了几个TX Transaction传输过程状态信息：

1. TX传输前，计算此次需要写入BRAM多少轮(Loop)，并写入相应寄存器内；

2. 主机开始写入；

3. 主机写入至BRAM完成后，将改变FPGA TX FSM状态。FPGA在每完成一次Loop发送后，将1中的寄存器自减，并置另一寄存器标志位为1，表示发送完成；

4. 主机轮询（设置超时时间3s）查询该寄存器标志位，复位并写入，FPGA TX FSM进入等待状态；

5. 重复2~4，直至全部传输完毕。