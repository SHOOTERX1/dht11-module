dht11-module
===
dht11温湿度传感器驱动
---
## 测试环境：
  1. 开发板：ROC-RK3328-CC
  2. Linux内核：[rockchip-linux:develop-4.4.](https://github.com/FireflyTeam/kernel/tree/roc-rk3328-cc)

### 仅学习内核驱动使用，使用软件的方式而非中断这会占用大量的cpu时间（读取过程中屏蔽了中断），使得CPU长时间处于忙等待。
