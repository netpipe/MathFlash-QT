#include <QtWidgets>
#include <QtSql>
#include <random>

struct Question
{
    QString text;
    QStringList choices;
    int correct;
};

class GameWindow : public QWidget
{
public:

    GameWindow(int lvl,QWidget *parent=nullptr)
        : QWidget(parent), level(lvl)
    {
        setWindowTitle("Math Flashcards");

        questionLabel=new QLabel;
        questionLabel->setAlignment(Qt::AlignCenter);

        scoreLabel=new QLabel("Score: 0");

        QVBoxLayout *layout=new QVBoxLayout(this);

        layout->addWidget(scoreLabel);
        layout->addWidget(questionLabel);

        for(int i=0;i<4;i++)
        {
            buttons[i]=new QPushButton;
            layout->addWidget(buttons[i]);

            connect(buttons[i],&QPushButton::clicked,this,[=]()
            {
                answer(i);
            });
        }

        generateQuestion();
    }

private:

    QLabel *questionLabel;
    QLabel *scoreLabel;
    QPushButton *buttons[4];

    Question current;

    int level;
    int score=0;

    std::mt19937 rng{std::random_device{}()};

    int randBetween(int a,int b)
    {
        std::uniform_int_distribution<int> d(a,b);
        return d(rng);
    }

    void generateQuestion()
    {
        int maxValue=10;

        switch(level)
        {
        case 1:maxValue=10;break;
        case 2:maxValue=20;break;
        case 3:maxValue=50;break;
        case 4:maxValue=100;break;
        case 5:maxValue=500;break;
        }

        int a=randBetween(1,maxValue);
        int b=randBetween(1,maxValue);

        int op=randBetween(0,2);

        int result;

        QString symbol;

        if(op==0)
        {
            result=a+b;
            symbol="+";
        }
        else if(op==1)
        {
            if(a<b)
                std::swap(a,b);

            result=a-b;
            symbol="-";
        }
        else
        {
            a=randBetween(2,maxValue/2+2);
            b=randBetween(2,12);
            result=a*b;
            symbol="×";
        }

        current.text=QString("%1 %2 %3 = ?")
                .arg(a)
                .arg(symbol)
                .arg(b);

        current.correct=randBetween(0,3);

        current.choices.clear();

        for(int i=0;i<4;i++)
            current.choices<<"";

        current.choices[current.correct]=QString::number(result);

        for(int i=0;i<4;i++)
        {
            if(i==current.correct)
                continue;

            int wrong;

            do
            {
                wrong=result+randBetween(-15,15);

                if(wrong==result)
                    wrong++;

            }while(current.choices.contains(QString::number(wrong)));

            current.choices[i]=QString::number(wrong);
        }

        questionLabel->setText(current.text);

        for(int i=0;i<4;i++)
            buttons[i]->setText(current.choices[i]);
    }

    void saveWrong()
    {
        QSqlQuery q;

        q.prepare(
        "INSERT INTO wrong_answers(question,correct_answer)"
        " VALUES(?,?)");

        q.addBindValue(current.text);
        q.addBindValue(current.choices[current.correct]);
        q.exec();
    }

    void answer(int chosen)
    {
        if(chosen==current.correct)
        {
            score++;
            scoreLabel->setText(QString("Score: %1").arg(score));
        }
        else
        {
            saveWrong();

            QMessageBox::information(
                        this,
                        "Wrong",
                        "Correct answer: "
                        +current.choices[current.correct]);
        }

        generateQuestion();
    }

};

class MenuWindow : public QWidget
{
public:

    MenuWindow()
    {
        setWindowTitle("Math Flashcards");

        QVBoxLayout *layout=new QVBoxLayout(this);

        QLabel *title=new QLabel("<h1>Math Flashcards</h1>");
        title->setAlignment(Qt::AlignCenter);

        layout->addWidget(title);

        for(int i=1;i<=5;i++)
        {
            QPushButton *b=new QPushButton(
                        QString("Level %1").arg(i));

            layout->addWidget(b);

            connect(b,&QPushButton::clicked,this,[=]()
            {
                GameWindow *g=new GameWindow(i);
                g->resize(350,300);
                g->show();
            });
        }

        QPushButton *stats=new QPushButton("Wrong Answers");

        layout->addWidget(stats);

        connect(stats,&QPushButton::clicked,this,[=]()
        {
            QString text;

            QSqlQuery q("SELECT question,correct_answer "
                        "FROM wrong_answers");

            while(q.next())
            {
                text+=q.value(0).toString();
                text+=" -> ";
                text+=q.value(1).toString();
                text+="\n";
            }

            QMessageBox::information(this,
                                     "Mistakes",
                                     text.isEmpty()
                                     ?"No mistakes."
                                     :text);
        });

    }

};

int main(int argc,char *argv[])
{
    QApplication app(argc,argv);

    QSqlDatabase db=
            QSqlDatabase::addDatabase("QSQLITE");

    db.setDatabaseName("flashcards.db");

    if(db.open())
    {
        QSqlQuery q;

        q.exec(
        "CREATE TABLE IF NOT EXISTS wrong_answers("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "question TEXT,"
        "correct_answer TEXT,"
        "time TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
    }

    MenuWindow w;
    w.resize(300,350);
    w.show();

    return app.exec();
}
