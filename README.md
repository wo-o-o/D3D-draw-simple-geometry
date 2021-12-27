# D3D-draw-simple-geometry
绘制简单几何体


绘制简单几何体大致流程
（若要以线框模式渲染几何体只需在pso描述中加上
    CD3DX12_RASTERIZER_DESC rsDesc(D3D12_DEFAULT);
    rsDesc.FillMode = D3D12_FILL_MODE_WIREFRAME; 
    rsDesc.CullMode = D3D12_CULL_MODE_NONE;
并将
 psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
更改为
 psoDesc.RasterizerState = rsDesc;）

各种创建和初始化
创建围栏并获取描述符的大小
检测质量支持，获取支持的全部显示模式
创建命令队列和命令列表
描述并创建交换链

创建描述符堆
创建渲染目标视图
创建深度/模板缓冲区及其视图

设置视口
设置裁剪矩形

创建存放常量缓冲区描述符等的描述符堆
创建常量缓冲区
创建根签名
创建顶点着色器，像素着色器和输入布局描述
创建想要绘制的几何体（顶点与索引）并复制到默认缓冲区
创建并设置pso


图形学的学习过程中，只看理论的话和相关数学算法的话，一方面是感觉没有结合到实践，同时算数学和啃英文书也很费脑子。
所以也有在参照3d game programming with DirectX-12跟着打代码（更费脑子····）。
把第四章的初始化Direct3D演示程序和第六章的绘制简单几何体传上来做个记录，做个备份。
对书中的代码进行了注释（书中只有少量英文注释）和纠错（debug了直接对临时变量进行取址等十来个小bug，现在可以在vs2019上直接运行了），一些d3d的细节和难以理解的地方也写注释里了，多少带有学习笔记的性质·····