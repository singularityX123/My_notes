# html的主要概念

#### 从标签到元素

`<`开头，`>`结尾，就是一个标签，例：

```c
<H1>
```

HTML的常见标签，及其含义：

HTML（超文本标记语言）使用各种标签来定义网页的内容结构。以下是一些常见的HTML标签及其含义：

1. `<html>`：定义HTML文档的根元素，包含整个页面的内容。
2. `<head>`：包含元信息（如编码、样式、脚本引用、页面标题等），不会在页面上直接显示。
3. `<title>`：定义网页的标题，通常显示在浏览器的标签页上。
4. `<body>`：包含网页的主体内容，所有可见的页面内容都放在这个标签内。
5. `<h1>`** - **`<h6>`：定义标题，`<h1>`是最高级别的标题，`<h6>`是最低级别的。
6. `<p>`：定义段落。
7. `<a>`：定义一个超链接，`href`属性指定链接的目标URL。
8. `<table>`：定义表格。
9. `<tr>`：定义表格中的行，用于`<table>`中。
10. `<td>`：定义表格中的单元格，用于`<tr>`中。
11. `<th>`：定义表格中的表头单元格。

..........

一对标签加上中间的内容就组成了一个元素,例：

```c
<H1>abcde</H1>
```

但也有一些元素是单个出现的，被称之为**空元素**。例：

```c
<br>
```

<br>表示换行

#### 从元素到属性
元素是HTML的基本单位，每个元素都可以指定一些属性，例如`<a>`表示一个超级链接，就是通过属性指定链接的：

```c
<a href="https://www.baidu.com/">链接文本</a>
```

很明显，除这种必不可少的属性外，也有一些其它常见的标签属性：

| <font style="color:rgb(0, 0, 0);">属性</font>     | <font style="color:rgb(0, 0, 0);">描述</font>                |
| ------------------------------------------------- | ------------------------------------------------------------ |
| <font style="color:rgb(51, 51, 51);">class</font> | <font style="color:rgb(51, 51, 51);">给元素指定一个或多个类名，方便通过 CSS 或 JavaScript 操作</font> |
| <font style="color:rgb(51, 51, 51);">id</font>    | <font style="color:rgb(51, 51, 51);">给元素一个唯一的标识符，可以用于 CSS 选择器或 JavaScript 操作</font> |
| <font style="color:rgb(51, 51, 51);">style</font> | <font style="color:rgb(51, 51, 51);">直接为元素定义 CSS 样式。</font> |
| <font style="color:rgb(51, 51, 51);">title</font> | <font style="color:rgb(51, 51, 51);">提供关于元素的额外信息，通常在鼠标悬停时显示。</font> |


#### 从属性到样式
1. <font style="color:rgb(51, 51, 51);">内联样式</font>

<font style="color:rgb(51, 51, 51);">当特殊的样式需要应用到个别元素时，就可以使用内联样式。 使用内联样式的方法是在相关的标签中使用样式属性。样式属性可以包含任何 CSS 属性。以下实例显示出如何改变段落的颜色和左外边距。</font>

```c
<p style="color:blue;margin-left:20px;">这是一个段落。</p>
```

2. <font style="color:rgb(51, 51, 51);">内部样式表</font>

<font style="color:rgb(51, 51, 51);">当单个文件需要特别样式时，就可以使用内部样式表。你可以在<head> 部分通过 <style>标签定义内部样式表:</font>

```c
<head>
<style type="text/css">
body {background-color:yellow;}
p {color:blue;}
</style>
</head>
```

3. <font style="color:rgb(51, 51, 51);">外部样式表</font>

<font style="color:rgb(51, 51, 51);">当样式需要被应用到很多页面的时候，外部样式表将是理想的选择。使用外部样式表，你就可以通过更改一个文件来改变整个站点的外观。</font>

```html
<head>
  <link rel="stylesheet" type="text/css" href="mystyle.css">
</head>
```

#### html相关学习资料
html示例代码：

```html
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>我的html页面</title>
</head>
<body>
    <h1>我的第一个标题</h1>
    <p>我的第一个段落。</p>
</body>
</html>
```

html语言标准官方文档：

[HTML Standard (whatwg.org)](https://html.spec.whatwg.org/multipage/)

html教程：  
[使用 HTML 构建 Web - 学习 Web 开发 | MDN (mozilla.org)](https://developer.mozilla.org/zh-CN/docs/Learn/HTML)

国内的优秀网站：  
[HTML 教程 | 菜鸟教程 (runoob.com)](https://www.runoob.com/html/html-tutorial.html)

