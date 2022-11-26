PCIe DMA手册17页，Legacy interrupts，PL使得中断置位user_irq_req，之后IP将发出user_irq_ack，表示已将此中断发送给PCIe host，之后host完成中断事务后复位PL内**有关中断的寄存器**，PL判断user_irq_ack  assert且**有关中断的寄存器**也改变了状态，表明host已处理完中断，则复位user_irq_req

user_irq_req需要保持置位直到user_irq_ack发出且中断事务结束。
