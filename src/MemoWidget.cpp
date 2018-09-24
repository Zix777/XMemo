#include "MemoWidget.h"

#ifdef __cplusplus
#include <hoedown/html.h>
#endif
#include <QDebug>
#include <QFile>
#include <sstream>

QString MarkdownToHtml(const QString &md) {
    auto *renderer =
            hoedown_html_renderer_new(static_cast<hoedown_html_flags>(0), 16);
    auto options = hoedown_extensions(
            HOEDOWN_EXT_TABLES | HOEDOWN_EXT_FENCED_CODE |
            HOEDOWN_EXT_HIGHLIGHT | HOEDOWN_EXT_AUTOLINK | HOEDOWN_EXT_QUOTE |
            HOEDOWN_EXT_MATH | HOEDOWN_EXT_MATH_EXPLICIT |
            HOEDOWN_EXT_STRIKETHROUGH);
    hoedown_document *document = hoedown_document_new(renderer, options, 16);
    QByteArray data = md.toUtf8();
    hoedown_buffer *buffer = hoedown_buffer_new(data.size());
    hoedown_document_render(document, buffer,
                            (const uint8_t *) data.constData(), data.size());
    hoedown_document_free(document);
    QString html = QString::fromUtf8(hoedown_buffer_cstr(buffer));
    hoedown_buffer_free(buffer);
    hoedown_html_renderer_free(renderer);
    qDebug() << html;
    return html;
}

MemoWidget::MemoWidget(MemoInfo *memoInfo, bool isEditMode, QWidget *parent)
        : QWidget(parent) {
    this->setObjectName("MemoWidget");
    this->memoInfo = memoInfo;
    this->color = memoInfo->getColor();
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(WIDTH, HEIGHT);
    this->installEventFilter(this);

    createCloseBtn();
    createNewBtn();
    createEditBtn();
    createPinBtn();
    createStayOnTopBtn();
    createContentEditor();
    createColorBtns();
    createContentView();

    loadStyleSheet(COLOR_TABLE.value(this->memoInfo->getColor()));

    if (isEditMode) {
        setMode(EDIT);
    } else {
        setMode(VIEW);
    }

    move(this->memoInfo->getPosition());
    show();
}

void MemoWidget::createCloseBtn() {
    closeBtn = new QPushButton(this);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setObjectName("closeBtn");  //在qss中指定部件时起作用
    closeBtn->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    closeBtn->move(this->width() - BUTTON_WIDTH, 0);
    closeBtn->setIcon(QIcon(":/image/widget/close.png"));
    closeBtn->setFlat(true);
    connect(closeBtn, &QPushButton::clicked, this, &MemoWidget::close);
    closeBtn->hide();
}

void MemoWidget::createNewBtn() {
    newBtn = new QPushButton(this);
    newBtn->setObjectName("newBtn");
    newBtn->setCursor(Qt::PointingHandCursor);
    newBtn->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    newBtn->move(0, 0);
    newBtn->setFlat(true);
    newBtn->setIcon(QIcon(":/image/widget/new.png"));
    connect(newBtn, &QPushButton::clicked, this, &MemoWidget::onNewBtnClicked);
    newBtn->hide();
}

void MemoWidget::createEditBtn() {

    editBtn = new QPushButton(this);
    editBtn->setObjectName("editBtn");
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    editBtn->move(BUTTON_WIDTH, 0);
    editBtn->setFlat(true);
    editBtn->setIcon(QIcon(":/image/widget/edit.png"));
    connect(editBtn, &QPushButton::clicked, this,
            &MemoWidget::onEditBtnClicked);
    editBtn->hide();
}

void MemoWidget::createPinBtn() {
    pinBtn = new QPushButton(this);
    pinBtn->setObjectName("pinBtn");
    pinBtn->setCursor(Qt::PointingHandCursor);
    pinBtn->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    pinBtn->move(BUTTON_WIDTH * 2, 0);
    pinBtn->setIcon(QIcon(":/image/widget/unpin.png"));
    pinBtn->setFlat(true);
    connect(pinBtn, &QPushButton::clicked, this, &MemoWidget::onPinBtnClicked);
    pinBtn->hide();
}

void MemoWidget::createStayOnTopBtn() {
    stayOnTopBtn = new QPushButton(this);
    stayOnTopBtn->setObjectName("stayOnTop");
    stayOnTopBtn->setCursor(Qt::PointingHandCursor);
    stayOnTopBtn->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
    stayOnTopBtn->move(BUTTON_WIDTH * 3, 0);
    stayOnTopBtn->setFlat(true);
    stayOnTopBtn->setIcon(QIcon(":/image/widget/stayOnTop.png"));
    connect(stayOnTopBtn, &QPushButton::clicked, this,
            &MemoWidget::onStayOnTopBtnClicked);
    stayOnTopBtn->hide();
}

void MemoWidget::createContentEditor() {
    contentEditor = new QPlainTextEdit(this);
    contentEditor->setObjectName("contentEditor");
    contentEditor->setFixedSize(WIDTH, HEIGHT - BUTTON_HEIGHT * 2);
    contentEditor->move(0, 0);
    contentEditor->setFrameStyle(QFrame::NoFrame);
    contentEditor->setPlainText(this->memoInfo->getContent());
    contentEditor->hide();
}

void MemoWidget::createColorBtns() {
    colorBtnsFame = new QFrame(this);
    colorBtnsFame->setFixedSize(WIDTH - 10, BUTTON_HEIGHT);
    colorBtnsFame->move(5, this->height() - int(BUTTON_HEIGHT * 1.5));
    for (int i = 0; i < COLOR_BUTTON_COUNT; ++i) {
        colorBtns[i] = new QPushButton(colorBtnsFame);
        colorBtns[i]->setObjectName("colorBtns");
        colorBtns[i]->setFixedSize(BUTTON_WIDTH, BUTTON_HEIGHT);
        colorBtns[i]->move(i * (BUTTON_WIDTH + 10), 0);

        colorBtns[i]->setIcon(
                QIcon(QString(":/image/widget/%1.png").arg(COLOR_TABLE[i])));
        colorBtns[i]->setIconSize(QSize(30, 30));
        colorBtns2Color.insert(static_cast<QObject *>(colorBtns[i]), i);
        connect(colorBtns[i], &QPushButton::clicked, this,
                &MemoWidget::onColorBtnClicked);
        colorBtns[i]->hide();
    }
}

void MemoWidget::createContentView() {
    contentView = new QWebEngineView(this);
    contentView->page()->setBackgroundColor(Qt::transparent);
    contentView->setObjectName("contentView");
    contentView->setFixedSize(WIDTH, HEIGHT - BUTTON_HEIGHT);
    contentView->move(0, BUTTON_HEIGHT);
    contentView->setAttribute(Qt::WA_TransparentForMouseEvents);
    contentView->setHtml(MarkdownToHtml(this->memoInfo->getContent()));
    contentView->hide();
}

void MemoWidget::closeEvent(QCloseEvent *) {
    emit closeMemo(this->memoInfo);
}

void MemoWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!isPinned && e->buttons() & Qt::LeftButton && isMoving) {
        move(e->globalPos() - relativePos);
    }
}

void MemoWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        relativePos = e->globalPos() - pos();
        isMoving = true;
    }
}

void MemoWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        isMoving = false;
        emit memoMoved(this->pos());
    }
}

void MemoWidget::setTopBtnVisibility(bool visibility) {
    closeBtn->setVisible(visibility);
    newBtn->setVisible(visibility);
    editBtn->setVisible(visibility);
    pinBtn->setVisible(visibility);
    stayOnTopBtn->setVisible(visibility);
}

void MemoWidget::setEditWidgetVisibility(bool visibility) {
    contentEditor->setVisible(visibility);
    for (auto &i : colorBtns) {
        i->setVisible(visibility);
    }
    if (visibility) {
        //切换激活窗口并置顶之，windows下可能无效
        this->show();
        this->activateWindow();
        this->raise();
        contentEditor->setFocus();
    }

    contentView->setVisible(!visibility);
}

bool MemoWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == this) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            this->onEditBtnClicked();
        } else if (event->type() == QEvent::WindowActivate) {
            if (memoInfo->getContent().isEmpty())
                setMode(EDIT);
            else
                setMode(SELECT);
            return true;
        } else if (event->type() == QEvent::WindowDeactivate) {
            save();
            setMode(VIEW);
            return true;
        }
    }
    return false;
}

MemoWidget::~MemoWidget() {
    delete contentView;
}

MemoInfo *MemoWidget::getMemoInfo() const {
    return memoInfo;
}

QString MemoWidget::getContent() const {
    return contentEditor->toPlainText();
}

void MemoWidget::setContent(QString content) {
    this->contentEditor->setPlainText(content);
    this->contentView->setHtml(content);
}

int MemoWidget::getColor() const {
    return color;
}

void MemoWidget::save() {
    contentView->setHtml(MarkdownToHtml(contentEditor->toPlainText()));
    emit memoChanged();
}

void MemoWidget::loadStyleSheet(const QString colorName) {
    QFile file(":/qss/" + colorName + ".qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());

    setStyleSheet(styleSheet);
}

void MemoWidget::setMode(Mode mode) {
    switch (mode) {
    case EDIT:
        setTopBtnVisibility(false);
        setEditWidgetVisibility(true);
        colorBtnsFame->show();
        break;
    case VIEW:
        setTopBtnVisibility(false);
        setEditWidgetVisibility(false);
        colorBtnsFame->hide();
        break;
    case SELECT:
        setTopBtnVisibility(true);
        setEditWidgetVisibility(false);
        colorBtnsFame->hide();
        break;
    default:
        break;
    }
}

void MemoWidget::onNewBtnClicked() {
    emit createMemo();
}

void MemoWidget::onEditBtnClicked() {
    setMode(EDIT);
}

void MemoWidget::onPinBtnClicked() {
    isPinned = !isPinned;
    if (isPinned) {
        pinBtn->setIcon(QIcon(":/image/widget/pin.png"));
    } else {
        pinBtn->setIcon(QIcon(":/image/widget/unpin.png"));
    }
}

void MemoWidget::onStayOnTopBtnClicked() {
    if (isStayOnTop) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint &
                       ~Qt::X11BypassWindowManagerHint);
        isStayOnTop = false;
    } else {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint |
                       Qt::X11BypassWindowManagerHint);
        isStayOnTop = true;
    }

    show();
}

/**
 * @brief MemoWidget::onColorBtnClicked 换肤
 */
void MemoWidget::onColorBtnClicked() {
    int color = colorBtns2Color.value(sender());
    this->color = color;
    QString colorName = COLOR_TABLE[color];
    loadStyleSheet(colorName);
    emit memoChanged();
}
